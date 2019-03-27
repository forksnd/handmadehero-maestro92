Handmade Hero Day 248 - Cleaning Up Data Block Display

Summary:
more thoughts on the debug system. Cleaned up the Data Block Display.

Keyword:
Debug System



3:34
so Casey mentioned that currently when we record data debug values we are using these functions 

                handmade_debug_interface.h 

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

for the "Value" variable, we are just copying the values into the debug_event by value. 
so right now, if we print all these debug_event that has data on our debug menu, it has no notion of "editing"


for example right now in our main loop, we have:


                win32_handmade.cpp

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {
                    ...
                    ...

                    while(GlobalRunning)
                    {
                        DEBUG_BEGIN_DATA_BLOCK(Platform_Controls, DEBUG_POINTER_ID(&DebugTimeMarkerIndex));
        ----------->    DEBUG_VALUE(GlobalPause);
                        DEBUG_VALUE(GlobalRenderingType);
                        DEBUG_END_DATA_BLOCK();


                        ...
                        ...
                    }

                    ...
                    ...
                }

so with the call to 

                DEBUG_VALUE(GlobalPause);
                DEBUG_VALUE(GlobalRenderingType);

we will be able to print them in our debug menu, but we wont be able to edit the value of GlobalPause and GlobalRenderingType
under our current debug system 


4:37
the notion of not "editing" is not necessarily a bad idea. there are 2 ways to do this, one is to snap a pointer to the thing. 


the reason why we arent gonna do that is because it would mean, we wont be able to ever output debug values that are transient.
once you copied the pointer, that means you have to leave that memory around till after the debug system is done with it, and our debug 
system right now is setup to debug systems forever. 

so we dont want to deal with the situation where we could be generating tons of debug data, and now we have all these memory that is
associated with all these frames of debug values, and we would like to get rid of them, but cant.

so for us, we just want to record a copy of value in calls like below:

                DEBUG_VALUE(GlobalPause);
                DEBUG_VALUE(GlobalRenderingType); 

                

5:44
what that means is that we need some more mechanism of doing the editing.
the idea is to use this thing that we were previously working on, which is utilizing this Debug_ID thing 

                DEBUG_BEGIN_DATA_BLOCK(Platform_Controls, DEBUG_POINTER_ID(&DebugTimeMarkerIndex));
                DEBUG_VALUE(GlobalPause);
                DEBUG_VALUE(GlobalRenderingType);
                DEBUG_END_DATA_BLOCK();


7:31
Casey got rid of the DEBUGInitializeValue(); call in handmade_debug_interface.h since we are no longer using them. 



19:36
after much contemplation Casey determined that for our debug calls 

                DEBUG_BEGIN_DATA_BLOCK(Platform_Controls, DEBUG_POINTER_ID(&DebugTimeMarkerIndex));

we will just use the string ("Platform_Controls" in this case); to be the unique identifier. we wont have any Debug_ID anymore 



35:14
Casey questions the way we are taking to store the debug frame data, 
right now we are using a linked list, which make accessing random debug frames very hard

                handmade_debug.cpp

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
            ---------------->   FreeOldestFrame(DebugState);
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

41:13
Casey questioning again how he now doesnt like our current design of the debug system reading in DataBlocks

