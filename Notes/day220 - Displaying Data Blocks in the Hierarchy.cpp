Handmade Hero Day 220 - Displaying Data Blocks in the Hierarchy

Summary:
making DEBUG_DATA_BLOCK work again, which allows us to view debug data for mouse picked entities
lots of debugging and code clean up 

Keyword:
debug system


16:26
so Recall Casey had the openData block code in handmade.cpp



                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...


                    if(DEBUG_REQUESTED(EntityDebugID))
                    {
                        DEBUG_BEGIN_DATA_BLOCK(Simulation_Entity, EntityDebugID); // , Entity->StorageIndex);
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

                        ...
                        ...

                        DEBUG_VALUE(Entity->WalkableDim);
                        DEBUG_VALUE(Entity->WalkableHeight);
                        DEBUG_END_DATA_BLOCK();
                    }
                }


so inside our CollateDebugRecords(); function, we have the following logic 

-   so the idea is that whenever we have a DEBUG_BEGIN_DATA_BLOCK, which gives us a DebugType_OpenDataBlock type, we create an OpenDataBlock.
    notice in the AllocateOpenDebugBlock(); function, we pass in the &Thread->FirstOpenDataBlock argument.

    then comes all the DEBUG_VALUE(); lines, which will all go to the "default" case. 

    in the default case, we check if we have a OpenDataBlock available. This is done by calling 
                    
                    if(Thread->FirstOpenDataBlock)


    If so, we put call StoreEvent(); on "Thread->FirstOpenDataBlock->Element"


-   we also call StoreEvent(); in DebugType_OpenDataBlock. This way we create an expandable forall the DEBUG_VALUE(); calls

-   full code below:

                handmade.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {       
                    ...
                    ...

                        case DebugType_OpenDataBlock:
                        {
                            open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                                DebugState, Element, FrameIndex, Event, &Thread->FirstOpenDataBlock);
                            StoreEvent(DebugState, Element, Event);
                        } break;

                        case DebugType_CloseDataBlock:
                        {
                            if(Thread->FirstOpenDataBlock)
                            {
                                StoreEvent(DebugState, Thread->FirstOpenDataBlock->Element, Event);

                                open_debug_block *MatchingBlock = Thread->FirstOpenDataBlock;
                                debug_event *OpeningEvent = MatchingBlock->OpeningEvent;
                                if(EventsMatch(*OpeningEvent, *Event))
                                {
                                    DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenDataBlock);
                                }
                            }
                        } break;

                        default:
                        {
                            debug_element *StorageElement = Element;
                            if(Thread->FirstOpenDataBlock)
                            {
                                StorageElement = Thread->FirstOpenDataBlock->Element;
                            }
                            StoreEvent(DebugState, StorageElement, Event);
                        } break;
                }


40:52
Created a function called DEBUGDrawElement();

                
                handmade_debug.cpp

                internal void DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id DebugID)
                {
                    debug_state *DebugState = Layout->DebugState;
                    
                    debug_stored_event *OldestEvent = Element->OldestEvent;
                    if(OldestEvent)
                    {
                        debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugID);
                        switch(OldestEvent->Event.Type)
                        {
                            case DebugType_CounterThreadList:
                            {
                                layout_element Element = BeginElementRectangle(Layout, &View->InlineBlock.Dim);
                                MakeElementSizable(&Element);
                                //                DefaultInteraction(&Element, ItemInteraction);
                                EndElement(&Element);

                                DrawProfileIn(DebugState, Element.Bounds, Layout->MouseP);
                            } break;

                            case DebugType_OpenDataBlock:
                            {
                                debug_stored_event *LastOpenBlock = OldestEvent;
                                for(debug_stored_event *Event = OldestEvent;
                                    Event;
                                    Event = Event->Next)
                                {
                                    if(Event->Event.Type == DebugType_OpenDataBlock)
                                    {
                                        LastOpenBlock = Event;
                                    }
                                }
                                
                                for(debug_stored_event *Event = LastOpenBlock;
                                    Event;
                                    Event = Event->Next)
                                {
                                    debug_id NewID = DebugIDFromGUID(Tree, Event->Event.GUID);
                                    DEBUGDrawEvent(Layout, Event, NewID);
                                }
                            } break;

                            default:
                            {
                                debug_stored_event *Event = Element->MostRecentEvent;
                                DEBUGDrawEvent(Layout, Event, DebugID);
                            } break;
                        }        
                    }
                }


lots of code clean up

