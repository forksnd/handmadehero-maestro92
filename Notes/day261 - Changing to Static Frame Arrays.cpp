Handmade Hero Day 261 - Changing to Static Frame Arrays

Summary:
Added function so that we are rendering profile nodes of multiple frames over time.

this way we can select a debug_event that was a few frames in the past.

because of the need of querying debug_events in the past frames. we changed our debug_frames storage. 
Now we are now storing a static array of debug_frames instead of just a linked list so we can index 
into a specific debug_frame better.


Keyword:
debug system, profiler 



5:09
Casey wants to be able to view perframe profile information as stacks

something that is designed to show "bars over time"


so Casey proceed to change DrawProfileBars(); into drawing vertical bars for multiple frames.

as you can see in the DrawFrameBars(); function, we pass in the FrameCount argument. 

                handmade_debug.cpp

                internal void DrawFrameBars(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                              debug_stored_event *FirstEvent, u32 FrameCount = 128)
                {
                    if(FrameCount > 0)
                    {
                        DebugState->MouseTextStackY = 10.0f;
                        
                        ...
                        ...
                        for(u32 FrameIndex = 0; RootEvent && (FrameIndex < FrameCount); ++FrameIndex, RootEvent = RootEvent->Next)
                        {
                            debug_profile_node *RootNode = &RootEvent->ProfileNode;
                            r32 FrameSpan = (r32)(RootNode->Duration);
                            r32 PixelSpan = GetDim(ProfileRect).y;

                            r32 Scale = 0.0f;
                            if(FrameSpan > 0)
                            {
                                Scale = PixelSpan / FrameSpan;
                            }

                            for(debug_stored_event *StoredEvent = RootNode->FirstChild;
                                StoredEvent;
                                StoredEvent = StoredEvent->ProfileNode.NextSameParent)
                            {
                                debug_profile_node *Node = &StoredEvent->ProfileNode;
                                debug_element *Element = Node->Element;
                                Assert(Element);

                                v3 Color = DebugColorTable[U32FromPointer(Element->GUID)%ArrayCount(DebugColorTable)];
                                r32 ThisMinY = ProfileRect.Min.y + Scale*(r32)(Node->ParentRelativeClock);
                                r32 ThisMaxY = ThisMinY + Scale*(r32)(Node->Duration);

                                rectangle2 RegionRect = RectMinMax(V2(AtX, ThisMinY), V2(AtX + BarWidth, ThisMaxY));
                                
                                PushRectOutline(&DebugState->RenderGroup, DebugState->UITransform, RegionRect,
                                    0.0f, V4(Color, 1), 2.0f);

                                if(IsInRectangle(RegionRect, MouseP))
                                {
                                    char TextBuffer[256];
                                    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "%s: %10ucy",
                                        Element->GUID, Node->Duration);
                                    DEBUGTextOutAt(MouseP + V2(0.0f, DebugState->MouseTextStackY), TextBuffer);
                                    DebugState->MouseTextStackY -= GetLineAdvance(DebugState);

                                    debug_interaction ZoomInteraction = {};
                                    ZoomInteraction.ID = GraphID;
                                    ZoomInteraction.Type = DebugInteraction_SetProfileGraphRoot;
                                    ZoomInteraction.Element = Element;
                                    DebugState->NextHotInteraction = ZoomInteraction;
                                }
                            }
                            
                            AtX += BarWidth;
                        }
                    }
                }



22:27
so when we are displaying profiler frames over time, we want to be able to click into an event and zoom into it. 

our current linked list structure makes the searching rather difficult. 

So Casey is saying instead of chaining all the frames together and freeing them dynamically,

recall currently we have 

                struct debug_element
                {
                    char *OriginalGUID; // NOTE(casey): Can never be printed!  Might point into unloaded DLL.
                    char *GUID;
                    u32 FileNameCount;
                    u32 LineNumber;
                    u32 NameStartsAt;
                    
                    b32 ValueWasEdited;
                    
                    debug_element *NextInHash;

    ----------->    debug_stored_event *OldestEvent;
                    debug_stored_event *MostRecentEvent;
                };



for each debug_element
we want to allocate a block, however many frames of backstore we want to keep. (lets say we want 128 or 256 frames)
and we just keep that 256 frames worth of stored events. That way we dont have to deal with the chaining in that way. 


26:40
the idea is that we got a frame index and a debug element, we want to be able to get the debug_stored_event that was 
for this debug_element for this frame. 





27:42
So Case added a new struct debug_element_frame
and in the debug_element, we just added a straight up array of debug_element_frame. 
                
                handmade_debug.h

                struct debug_element_frame
                {
                    u64 TotalClocks;
                    debug_stored_event *OldestEvent;
                    debug_stored_event *MostRecentEvent;
                };

                struct debug_element
                {
                    char *OriginalGUID; // NOTE(casey): Can never be printed!  Might point into unloaded DLL.
                    char *GUID;
                    u32 FileNameCount;
                    u32 LineNumber;
                    u32 NameStartsAt;
                    
                    b32 ValueWasEdited;
                    
                    debug_element_frame Frames[DEBUG_FRAME_COUNT];
                    
                    debug_element *NextInHash;
                };

30:10
then in debug_state, we just store DEBUG_FRAME_COUNT of backdata. 

                struct debug_state
                {
                    ...
                    ...

                    u32 TotalFrameCount;

                    u32 MostRecentFrameOrdinal;
                    u32 CollationFrameOrdinal;
                    u32 OldestFrameOrdinal;
                    debug_frame Frames[DEBUG_FRAME_COUNT];
                    
                    ...
                    ...
                };

recall previously we had 

                struct debug_state
                {
                    ...
                    ...

                    u32 TotalFrameCount;
                    u32 FrameCount;
                    debug_frame *OldestFrame;
                    debug_frame *MostRecentFrame;

                };

