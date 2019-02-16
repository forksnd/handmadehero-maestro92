Handmade Hero Day 219 - Automatically Constructed Debug Hierarchies

Summary:
created hierarchy for debug_element and debug_variable_group
the hierarchy is implied by the block names. the levels are inferred by the underscores

restored expand and collapse functionality debug_variable_group when we render the debug_UI

Keyword:
debug system, memory 


4:30
now that we have our debug_elements and debug_events stored. What we want to do now is to build
some user interface on top of 



5:57
so what we want to do in CollateDebugRecords is that, if we have a debug_event, and if there are some Hierarchical information about it 
we would want to build that hierarchy



18:26
so inside GetElementFromEvent(); when we see a debug_event for the first time,
we will create a debug_element for it.

that will be also the time where we want to do a Hierarchical lookup
so what we do is we first look up its ParentGroup by calling GetGroupForHierarchicalName(); 
then we add it to that parentGroup 


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

                        debug_variable_group *ParentGroup =
    --------------->        GetGroupForHierarchicalName(DebugState, DebugState->RootGroup, Event->BlockName);
    --------------->    AddElementToGroup(DebugState, ParentGroup, Result);
                    }

                    return(Result);
                }



we now look at the GetGroupForHierarchicalName(); function

Recall that we built the hierarchy into the name 


for example:
                case EntityType_Space:
                {
                    DEBUG_IF(Simulation_UseSpaceOutlines)
                    {
                        for(uint32 VolumeIndex = 0;
                            VolumeIndex < Entity->Collision->VolumeCount;
                            ++VolumeIndex)
                        {
                            sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        
                            PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, V4(0, 0.5f, 1.0f, 1));
                        }
                    }
                } break;


for the debug variable "Simulation_UseSpaceOutlines", the hierarchy is 

        Simulation 
            UseSpaceOutlines

so in the GetGroupForHierarchicalName(); we just scan through our Name and find the first underscore. 
once we found an underscore, we GetOrCreateGroupWithName();

so take the Simulation_UseSpaceOutlines example:

SubGroup will be "Simulation", so we create a group that is Simulation

then we call GetGroupForHierarchicalName(DebugState, Simulation (as a SubGroup), "UseSpaceOutlines")


if we cant find a underscore, then DebugState->RootGroup is our parent. as you have seen in the GetElementFromEvent(); function,
we pass in "DebugState->RootGroup" as the parent



-   full code below:

                internal debug_variable_group * GetGroupForHierarchicalName(debug_state *DebugState, debug_variable_group *Parent, char *Name)
                {
                    debug_variable_group *Result = Parent;

                    char *FirstUnderscore = 0;
                    for(char *Scan = Name;
                        *Scan;
                        ++Scan)
                    {
                        if(*Scan == '_')
                        {
                            FirstUnderscore = Scan;
                            break;
                        }
                    }

                    if(FirstUnderscore)
                    {
                        debug_variable_group *SubGroup = GetOrCreateGroupWithName(DebugState, Parent, (u32)(FirstUnderscore - Name), Name);
                        Result = GetGroupForHierarchicalName(DebugState, SubGroup, FirstUnderscore + 1);
                    }
                    
                    return(Result);
                }




39:59
so now we look at the GetOrCreateGroupWithName(); function 

essentially we are doing a string match

                handmade_debug.cpp

                internal debug_variable_group * GetOrCreateGroupWithName(debug_state *DebugState, debug_variable_group *Parent, u32 NameLength, char *Name)
                {
                    debug_variable_group *Result = 0;
                    for(debug_variable_link *Link = Parent->Sentinel.Next;
                        Link != &Parent->Sentinel;
                        Link = Link->Next)
                    {
                        if(Link->Children && StringsAreEqual(Link->Children->NameLength, Link->Children->Name,
                                                             NameLength, Name))
                        {
                            Result = Link->Children;
                        }
                    }

                    if(!Result)
                    {
                        Result = CreateVariableGroup(DebugState, NameLength, Name);
                        AddGroupToGroup(DebugState, Parent, Result);
                    }

                    return(Result);
                }



just to recap again, our structure is below:

                handmade_debug.cpp


                struct debug_variable_group;
                struct debug_variable_link
                {
                    debug_variable_link *Next;
                    debug_variable_link *Prev;

                    debug_variable_group *Children;
                    debug_element *Element;
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
                    u32 NameLength;
                    char *Name;
                    debug_variable_link Sentinel;
                };


takeing the graph from day 210

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


58:36
restored the ability to be able to DebugInteraction_ToggleExpansion

                handmade_debug.cpp

                internal void DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    switch(DebugState->Interaction.Type)
                    {
    ------------->      case DebugInteraction_ToggleExpansion:
                        {
                            debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugState->Interaction.ID);
                            View->Collapsible.ExpandedAlways = !View->Collapsible.ExpandedAlways;
                        } break;
                        
                        case DebugInteraction_ToggleValue:
                        {
                            debug_event *Event = DebugState->Interaction.Event;
                            Assert(Event);
                            switch(Event->Type)
                            {
                                case DebugType_b32:
                                {
                                    Event->Value_b32 = !Event->Value_b32;
                                } break;
                            }
                        } break;
                    }

                    WriteHandmadeConfig(DebugState);

                    DebugState->Interaction.Type = DebugInteraction_None;
                    DebugState->Interaction.Generic = 0;
                }

And Casey added the DebugInteraction_ToggleExpansion type 

                handmade_debug.h                

                enum debug_interaction_type
                {
                    DebugInteraction_None,

                    DebugInteraction_NOP,

                    DebugInteraction_AutoModifyVariable,
                    
                    DebugInteraction_ToggleValue,
                    DebugInteraction_DragValue,
                    DebugInteraction_TearValue,

                    DebugInteraction_Resize,
                    DebugInteraction_Move,

                    DebugInteraction_Select,

    ----------->    DebugInteraction_ToggleExpansion,
                };

