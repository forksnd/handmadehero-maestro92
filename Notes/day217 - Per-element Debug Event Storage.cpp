Handmade Hero Day 217 - Per-element Debug Event Storage

Summary:
code clean up, wrote the struct for debug_element. 
debug_element will now store a list of debug_events by frames.

explained his view on MVC pattern for UI

Keyword:
debug system



7:27
Casey continuing on the debug refactor ideas he had in day 216. In which he wants to introduce
the concept of debug_element and now he creates it 
[I feel like Casey is just re-writing debug_variable?]

                handmade_debug.h

                struct debug_stored_event
                {
                    union
                    {
                        debug_stored_event *Next;
                        debug_stored_event *NextFree;
                    };

                    u32 FrameIndex;
                    debug_event Event;
                };

                struct debug_element
                {
                    debug_element *NextInHash;
                    
                    debug_stored_event *OldestEvent;
                    debug_stored_event *MostRecentEvent;
                };


so the debug_element will just have a chain of debug_event. kind of like storing debug_events of all the past frames.

we are storing it as a linked list, so this way we can free the oldest event easily.



33:01
added a hash table for all the debug_elements
                
                handmade_debug.h

                struct debug_state
                {
                    ...
                    debug_element *ElementHash[1024];
                    ...
                };


41:36
Casey writing the new FreeFrame(); function
essentially we go through our debug_element hash table. 
we go through all of the debug_events inside the debug_element, if the debug_event FrameIndex is below or equal debug_frame->FrameIndex,
then we free it. 

essentially we are deleting debug_events that occured before Frame->FrameIndex

The debug_events inside the debug_element, as mentioned above, is a linkedlist structure.
so we iterate the linkedlist until we reach a frame that is larger than Frame

                while(Element->OldestEvent && (Element->OldestEvent->FrameIndex <= Frame->FrameIndex))


-   full code below: 


                handmade_debug.cpp 

                internal void FreeFrame(debug_state *DebugState, debug_frame *Frame)
                {
                    for(u32 ElementHashIndex = 0; ElementHashIndex < ArrayCount(DebugState->ElementHash); ++ElementHashIndex)
                    {
                        for(debug_element *Element = DebugState->ElementHash[ElementHashIndex];
                            Element;
                            Element = Element->NextInHash)
                        {
                            while(Element->OldestEvent && (Element->OldestEvent->FrameIndex <= Frame->FrameIndex))
                            {
                                debug_stored_event *FreeEvent = Element->OldestEvent;
                                Element->OldestEvent = FreeEvent->Next;
                                if(Element->MostRecentEvent == FreeEvent)
                                {
                                    Assert(FreeEvent->Next == 0);
                                    Element->MostRecentEvent = 0;
                                }
                                
                                FREELIST_DEALLOCATE(FreeEvent, DebugState->FirstFreeStoredEvent);
                            }
                        }
                    }

                    FREELIST_DEALLOCATE(Frame, DebugState->FirstFreeFrame);
                }



59:12
created a unique GUID for the debug_event

essentially Casey created a string that concatenates __FILE__, __LINE__ and __COUNTER__ together

                handmade_debug_interface.h


                #define UniqueFileCounterString__(A, B, C) A "(" #B ")." #C
                #define UniqueFileCounterString_(A, B, C) UniqueFileCounterString__(A, B, C)
                #define UniqueFileCounterString(A, B, C) UniqueFileCounterString_(A, B, C)

                #define RecordDebugEvent(EventType, Block)           \
                        u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
                        u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;            \
                        Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0]));   \
                        debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; \
                        Event->Clock = __rdtsc();                       \
                        Event->Type = (u8)EventType;                                    \
                        Event->CoreIndex = 0;                                           \
                        Event->ThreadID = (u16)GetThreadID();                         \
    ---------------->   Event->GUID = UniqueFileCounterString(__FILE__, __LINE__, __COUNTER__) \
                        Event->BlockName = Block;                              \
                    

Q/A
1:03:56
what do you think about the model-view-controller pattern?

Casey thinks its close to being the right way to look at UI. Or it is a good way to look at UI.
It is slightly off. The view part makes sense. The model part makes sense.
Casey has a bit of problem with the controller part. The controller part is often wielded into in the view.
its hard to decouple these two. Its really like model-view-input processor.

MVC makes it sound like the controller part is separate from the view part