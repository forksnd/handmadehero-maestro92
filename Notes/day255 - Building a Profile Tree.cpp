Handmade Hero Day 255 - Building a Profile Tree

Summary:
added the debug_profile_node struct to support the Profile Tree

(not fully finished, will be finished tomorrow);

Keyword:
Debug System


4:06
Casey mentioned that we plan to address the problem of profile queries taking too long. 



15:03
inside the debug_frame, Casey added some more information 


particularly, we added 

                u32 StoredEventCount;
                u32 ProfileBlockCount;
                u32 DataBlockCount;


-   full code below:

                #define MAX_REGIONS_PER_FRAME 2*4096
                struct debug_frame
                {
                    // IMPORTANT(casey): This actually gets freed as a set in FreeFrame!

                    union
                    {
                        debug_frame *Next;
                        debug_frame *NextFree;
                    };

                    u64 BeginClock;
                    u64 EndClock;
                    r32 WallSecondsElapsed;

                    r32 FrameBarScale;

                    u32 FrameIndex;
                    
    ----------->    u32 StoredEventCount;
                    u32 ProfileBlockCount;
                    u32 DataBlockCount;
                    
                    debug_stored_event *RootProfileNode;
                };

so each time, we collate, we would want to update the information in debug_frame


now that whenever we get a DebugType_BeginBlock or DebugType_OpenDataBlock, we increase the counts 

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {  
                    case DebugType_BeginBlock:
                    {
                        ++DebugState->CollationFrame->ProfileBlockCount;
                    }
                    ...
                    ...

                    case DebugType_OpenDataBlock:
                    {
                        ++DebugState->CollationFrame->DataBlockCount;
                    }
                    ...
                    ...
                }


19:25
Casey printed out the number of stored events and number of ~2000 profile blocks.


21:50
Casey looking at the CollateDebugRecords(); function and thinking about how we would want to process this data.

one thing is the tree/hiearchical view, the other thing we need to address is where there is a lot of calls to a function. 

one of Casey_s concern is that when we have a lot of calls to a function, Casey doesnt know how we are gonna display it. 

for instance, we may have thousands and bilions of a function call, and there is no way for us to render it.
so we need some way of coalesing them. 

Essentially, we need to be able to detect the case when there is too many calls to a function and we are not gonna render it 


26:02 
Casey added the debug_profile_node to help with the "tree" profiling view
                
                handmade_debug.h

                struct debug_profile_node
                {
                    struct debug_element *Element;
                    struct debug_stored_event *FirstChild;
                    struct debug_stored_event *NextSameParent;
                    u32 ParentRelativeClock;
                    u32 Duration;
                    u32 AggregateCount;
                    u16 ThreadOrdinal;
                    u16 CoreIndex;
                };



28:02
then in our debug_stored_event struct, we have it store the debug_profile_node
                
                handmade_debug.h

                struct debug_stored_event
                {
                    union
                    {
                        debug_stored_event *Next;
                        debug_stored_event *NextFree;
                    };

                    u32 FrameIndex;
                    
                    union
                    {
                        debug_event Event;
                        debug_profile_node ProfileNode;
                    };
                };



1:03:05
Whenever we run into a DebugType_BeginBlock, we see if we have a ParentEvent
if so, we assign the parent, child pointers properly 

in the "else if(!ParentEvent)" case 
which means we didnt have a parent, we initalize a new parent.



                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {    
                    ...
                    ...

                    switch(Event->Type)
                    {
                        case DebugType_BeginBlock:
                        {
                            ++DebugState->CollationFrame->ProfileBlockCount;
                            debug_element *Element = GetElementFromEvent(DebugState, Event, DebugState->ProfileGroup, false);
                             
        --------------->    debug_stored_event *ParentEvent = DebugState->CollationFrame->RootProfileNode;
                            u64 ClockBasis = DebugState->CollationFrame->BeginClock;
                            if(Thread->FirstOpenCodeBlock)
                            {
                                ParentEvent = Thread->FirstOpenCodeBlock->Node;
                                ClockBasis = Thread->FirstOpenCodeBlock->OpeningEvent->Clock;
                            }
                            else if(!ParentEvent)
                            {
                                debug_event NullEvent = {};
                                ParentEvent = StoreEvent(DebugState, Element, &NullEvent);
        ------------------->    debug_profile_node *Node = &ParentEvent->ProfileNode;
                                Node->Element = 0;
                                Node->FirstChild = 0;
                                Node->NextSameParent = 0;
                                Node->ParentRelativeClock = 0;
                                Node->Duration = (u32)(DebugState->CollationFrame->EndClock -
                                        DebugState->CollationFrame->BeginClock);
                                Node->AggregateCount = 0;
                                Node->ThreadOrdinal = 0;
                                Node->CoreIndex = 0;

                                DebugState->CollationFrame->RootProfileNode = ParentEvent;
                            }
                                                    
                            debug_stored_event *StoredEvent = StoreEvent(DebugState, Element, Event);
                            debug_profile_node *Node = &StoredEvent->ProfileNode;
                            Node->Element = Element;
                            Node->FirstChild = 0;
                            Node->ParentRelativeClock = (u32)(Event->Clock - ClockBasis);
                            Node->Duration = 0;
                            Node->AggregateCount = 0;
                            Node->ThreadOrdinal = (u16)Thread->LaneIndex;
                            Node->CoreIndex = Event->CoreIndex;
                            
                            Node->NextSameParent = ParentEvent->ProfileNode.FirstChild;
        --------------->    ParentEvent->ProfileNode.FirstChild = StoredEvent;
                            
                            open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                                DebugState, Element, FrameIndex, Event, &Thread->FirstOpenCodeBlock);
                            DebugBlock->Node = StoredEvent;
                        } break;
