Handmade Hero Day 218 - Hashing Debug Elements

Summary:
hashed debug elements into our debug_state hash table. The hash key is the pointer to our GUID strings
corrected the memory allocation for debug_elements and debug_frames that we first wrote in day 216
stored debug_events in debug_elements

Keyword:
debug system, memory 

7:25
made it so that the RecordDebugEvent now takes in a GUID

                inline debug_event DEBUGInitializeValue(debug_type Type, debug_event *SubEvent, char *GUID, char *Name)
                {
                    RecordDebugEvent(DebugType_MarkDebugValue, "");
                    Event->Value_debug_event = SubEvent;

                    SubEvent->Clock = 0;
                    SubEvent->GUID = GUID;
                    SubEvent->BlockName = Name;
                    SubEvent->ThreadID = 0;
                    SubEvent->CoreIndex = 0;
                    SubEvent->Type = (u8)Type;

                    return(*SubEvent);
                }

                #define DEBUG_IF__(Path)  \
                    local_persist debug_event DebugValue##Path = DEBUGInitializeValue((DebugValue##Path.Value_b32 = GlobalConstants_##Path, DebugType_b32), &DebugValue##Path, UniqueFileCounterString(), #Path); \
                    if(DebugValue##Path.Value_b32)

                #define DEBUG_VARIABLE__(type, Path, Variable)                          \
                    local_persist debug_event DebugValue##Variable = DEBUGInitializeValue((DebugValue##Variable.Value_##type = GlobalConstants_##Path##_##Variable, DebugType_##type), &DebugValue##Variable, UniqueFileCounterString(), #Path "_" #Variable); \
                    type Variable = DebugValue##Variable.Value_##type;


8:48
mentions that the PushSizeWithDeallocation(); function that we did on day 216 is incorrect.


10:43
So Casey will now try to put debug_elements in to the debug_state.ElementHash variable 

                handmade_debug.h

                struct debug_state
                {
                    ...
                    debug_element* ElementHash[1024];
                    ...
                };

the one nice thing about our current setup is that we have a unique GUID for every debug_event


                #define UniqueFileCounterString__(A, B, C) A "(" #B ")." #C
                #define UniqueFileCounterString_(A, B, C) UniqueFileCounterString__(A, B, C)
                #define UniqueFileCounterString() UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__)

this concatenated string is created at compile time. So when it gets created, its gonna be put into the string table of the executable 
[I suppose that will be in the data segment or data section if you do a dumpbin of a .o file], and becuz it is a unique string,
that means every single one of these string will get a unique pointer in our program. Which means we dont actually have to Hash the string,
which will be relatively slow. 

[if we were to hash the string, that means we have to somehow convert the string into a numerical value. 
    so one way you can imagine is to loop through all the char and sum up the ASCII number of do some calculations with it]

So what that means that we just need to hash the pointer to the string, and we know that no two strings that are generated this way will 
ever collapse into the same pointer. 


12:07
so in the CollateDebugRecords(); function, when we loop through the debug_event, and we try to retrieve its corresponding debug_element


                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {    
                    for(u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
                    {
                        debug_event *Event = EventArray + EventIndex;
                        debug_element *Element = GetElementFromEvent(DebugState, Event);

                        ...
                        ...
                    }
                }


so in the GetElementFromEvent(); we just look up in the hash table.

-   notice that Casey made a hash value by doing 

                u32 HashValue = (u32)((memory_index)Event->GUID >> 2);


-   also notice that when we are searching in the hash bucket, we are not comparing strings 
    we are just comparing GUID

                if(Chain->GUID == Event->GUID)
                {
                    ...
                    ...
                }

-   if we cant find it in our hash table,
    we just create a new entry in our hashtable.

-   notice that in this function, when we use the debug_event to get the debug_element from a hash bucket,
    we dont actually store the debug event inside that debug_element
    that is done later             


-   full code below:

                handmade_debug.cpp

                internal debug_element * GetElementFromEvent(debug_state *DebugState, debug_event *Event)
                {
                    Assert(Event->GUID);
                    
                    u32 HashValue = (u32)((memory_index)Event->GUID >> 2);
                    // TODO(casey): Verify this turns into an and (not a mod)
                    u32 Index = (HashValue % ArrayCount(DebugState->ElementHash));

                    debug_element *Result = 0;
                    
                    for(debug_element *Chain = DebugState->ElementHash[Index];
                        Chain;
                        Chain = Chain->NextInHash)
                    {
                        if(Chain->GUID == Event->GUID)
                        {
                            Result = Chain;
                            break;
                        }
                    }

                    if(!Result)
                    {
                        Result = PushStruct(&DebugState->DebugArena, debug_element);

                        Result->GUID = Event->GUID;
                        Result->NextInHash = DebugState->ElementHash[Index];
                        DebugState->ElementHash[Index] = Result;

                        Result->OldestEvent = Result->MostRecentEvent = 0;
                    }

                    return(Result);
                }



as mentioned above, once we go through most of the work, we then finally store the debug_event inside the debug_element

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {   

                    debug_event *Event = EventArray + EventIndex;
                    debug_element *Element = GetElementFromEvent(DebugState, Event);


                    for(u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
                    {

                        ...
                        ...

                        default:
                        {
                            StoreEvent(DebugState, Element, Event);
                        } break;
                    }
                }

here is the StoreEvent function. 
Recall that inside debug_element, we have a linked list of debug_stored_event that is sorted by frame number.
we keep track of the oldest and newest frames. 


                internal debug_stored_event * StoreEvent(debug_state *DebugState, debug_element *Element, debug_event *Event)
                {
                    debug_stored_event *Result = 0;
                    while(!Result)
                    {
                        Result = DebugState->FirstFreeStoredEvent;
                        if(Result)
                        {
                            DebugState->FirstFreeStoredEvent = Result->NextFree;
                        }
                        else
                        {
                            if(ArenaHasRoomFor(&DebugState->PerFrameArena, sizeof(debug_stored_event)))
                            {
                                Result = PushStruct(&DebugState->PerFrameArena, debug_stored_event);
                            }
                            else
                            {
                                Assert(DebugState->OldestFrame);    
                                FreeOldestFrame(DebugState);
                            }
                        }
                    }
                    
                    Result->Next = 0;
                    Result->FrameIndex = DebugState->CollationFrame->FrameIndex;
                    Result->Event = *Event;

                    if(Element->MostRecentEvent)
                    {
                        Element->MostRecentEvent = Element->MostRecentEvent->Next = Result;
                    }
                    else
                    {
                        Element->OldestEvent = Element->MostRecentEvent = Result;
                    }

                    return(Result);
                }


33:29
so Casey completely deleted the PushSizeWithDeallocation(); function from day 216, and he rewrote the 
NewFrame(); and FreeFrame(); function which deallocates frame memory

-   if we can get some from the freelist, we get from the free list.

-   if not, we just PushStruct(); to our arena

-   if we dont have room for it, we just free the oldest frame


                internal debug_frame * NewFrame(debug_state *DebugState, u64 BeginClock)
                {
                    debug_frame *Result = 0;
                    while(!Result)
                    {
                        Result = DebugState->FirstFreeFrame;
                        if(Result)
                        {
                            DebugState->FirstFreeFrame = Result->NextFree;
                        }
                        else
                        {
                            if(ArenaHasRoomFor(&DebugState->PerFrameArena, sizeof(debug_frame)))
                            {
                                Result = PushStruct(&DebugState->PerFrameArena, debug_frame);
                            }
                            else
                            {
                                Assert(DebugState->OldestFrame);    
                                FreeOldestFrame(DebugState);
                            }
                        }
                    }

                    ZeroStruct(*Result);
                    Result->FrameIndex = DebugState->TotalFrameCount++;
                    Result->FrameBarScale = 1.0f;
                    Result->BeginClock = BeginClock;

                    return(Result);
                }


and we have the FreeFrame(); function


                handmade_debug.cpp

                internal void FreeFrame(debug_state *DebugState, debug_frame *Frame)
                {
                    for(u32 ElementHashIndex = 0;
                        ElementHashIndex < ArrayCount(DebugState->ElementHash);
                        ++ElementHashIndex)
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



we also have a FreeOldestFrame(); function which also calls FreeFrame();

                internal void FreeOldestFrame(debug_state *DebugState)
                {
                    if(DebugState->OldestFrame)
                    {
                        debug_frame *Frame = DebugState->OldestFrame;
                        DebugState->OldestFrame = Frame->Next;
                        if(DebugState->MostRecentFrame == Frame)
                        {
                            Assert(Frame->Next == 0);
                            DebugState->MostRecentFrame = 0;
                        }
                            
                        FreeFrame(DebugState, Frame);
                    }
                }



43:01
assigned some subArena for debug frames in the debug initalization 

                handmade_debug.cpp

                internal void DEBUGStart(debug_state *DebugState, game_assets *Assets, u32 Width, u32 Height)
                {
                    TIMED_FUNCTION();

                    if(!DebugState->Initialized)
                    {
                        DebugState->FrameBarLaneCount = 0;
                        DebugState->FirstThread = 0;
                        DebugState->FirstFreeThread = 0;
                        DebugState->FirstFreeBlock = 0;

                        DebugState->FrameCount = 0;    

                        DebugState->OldestFrame = DebugState->MostRecentFrame = DebugState->FirstFreeFrame = 0;
                        DebugState->CollationFrame = 0;

                        ...
                        ...

                        memory_index TotalMemorySize = DebugGlobalMemory->DebugStorageSize - sizeof(debug_state);
                        InitializeArena(&DebugState->DebugArena, TotalMemorySize, DebugState + 1);
                        SubArena(&DebugState->PerFrameArena, &DebugState->DebugArena, (TotalMemorySize / 2));
                    }
                }
