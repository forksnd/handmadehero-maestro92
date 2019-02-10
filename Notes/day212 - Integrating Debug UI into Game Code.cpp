Handmade Hero Day 212 - Integrating Debug UI into Game Code

Summary:
cleaned up the "the mouse picking to examine entity" logic in the game code. 
made it so that the debug system has more control over the "mouse picking to examine entity" logic.

previously, the debug system ui only display entitys if the mouse hovers on it. Now we added tech 
so that when you left click, the debug system remembers it and keeps on displaying that entity_s information

added support for DebugInteraction_Select

Keyword:
debug system



4:51
Casey addresses the problem of if I select something on the UI layer, the it will also select
object in the world simulation that is underneath

so the UI layer and the world simulation layer needs a unified understanding of what a debug interaction is 
and who is doing stuff with it 



8:41
Casey addresses that currently the way examine moused picked entity is when your mouse hovers on it,
it will udpate the numbers and debug information on the UI. But if you stop hovering on it,
it will no longer update. 

Casey wants it so that once you click on the, the debug system remembers that you are examining this entity.



10:35
so Casey refactored that part of the code  

-   when we do a hit test with the entity, we will annouce it to the debug system 
    notice that this is a two part system         
    

                    if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                       (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                    {
                        DEBUG_HIT(EntityDebugID, LocalMouseP.z);
                    }

                    v4 OutlineColor;
                    if(DEBUG_HIGHLIGHTED(EntityDebugID, &OutlineColor))
                    {
                        PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);
                    }

    DEBUG_HIT(); is the once that will annouce to the debug system that this entity has been hit 
    
    DEBUG_HIGHLIGHTED(); is a flag told by the debug system whether we want ot highlight MousePicked entities     


-   then if the debug system wants data on the debug entity, we will give stream out the values to the debug system
                
                    if(DEBUG_REQUESTED(EntityDebugID))
                    {
                        ...
                        ...
                    }


-   full code below:

                if(DEBUG_UI_ENABLED)
                {
                    debug_id EntityDebugID = DEBUG_POINTER_ID(GameState->LowEntities + Entity->StorageIndex);
                    
                    for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
                    {
                        sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                        v3 LocalMouseP = Unproject(RenderGroup, MouseP);

                        if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                           (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                        {
                            DEBUG_HIT(EntityDebugID, LocalMouseP.z);
                        }

                        v4 OutlineColor;
                        if(DEBUG_HIGHLIGHTED(EntityDebugID, &OutlineColor))
                        {
                            PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);
                        }
                    }

                    if(DEBUG_REQUESTED(EntityDebugID))
                    {
                        DEBUG_BEGIN_DATA_BLOCK("Simulation Entity", EntityDebugID);
                        DEBUG_VALUE(Entity->StorageIndex);
                        ...
                        ...
                        DEBUG_END_DATA_BLOCK();
                    }
                }


15:25
so notice in the code above, we are calling DEBUG_POINTER_ID(); and we are passing a pointer to the function
to get a debug_id back.


                debug_id EntityDebugID = DEBUG_POINTER_ID(GameState->LowEntities + Entity->StorageIndex);


the actualy function is defined in handmade_debug_interface.h 

                handmade_debug_interface.h                

                inline debug_id DEBUG_POINTER_ID(void *Pointer)
                {
                    debug_id ID = {Pointer};
                    
                    return(ID);
                }

the idea is that we know that the pointer value will be unique, so Casey will use it as a unique id for the debug system 
to associate to this entity.


33:20
Casey now implementing these DEBUG functions
these should be pretty straight forward. the DebugSystem just the remembers the entity you remember 
as the DebugState->NextHotInteraction

then this goes back to our debug system User interaction logic: 
when the DebugSystem ticks(), it see if you have left clicked. if so NextHotInteraction becomes HotInteraction

In DEBUG_HIGHLIGHTED(); and DEBUG_REQUESTED(); it spretty much compare the debug_id with the HotInteraction.ID
everything should be pretty straight forward.

                handmade_debug.cpp

                internal void DEBUG_HIT(debug_id ID, r32 ZValue)
                {
                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        DebugState->NextHotInteraction = DebugIDInteraction(DebugInteraction_Select, ID);
                    }
                }

                internal b32 DEBUG_HIGHLIGHTED(debug_id ID, v4 *Color)
                {
                    b32 Result = false;
                    
                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        if(IsSelected(DebugState, ID))
                        {
                            *Color = V4(0, 1, 1, 1);
                            Result = true;
                        }

                        if(DebugIDsAreEqual(DebugState->HotInteraction.ID, ID))
                        {
                            *Color = V4(1, 1, 0, 1);
                            Result = true;
                        }
                    }

                    return(Result);
                }


notice in the DEBUG_REQUESTED(); function, our condition is IsSelected(); || DebugIDsAreEqual();
the idea is that we will display the debug info of both entities that are selected and entities that are hovered.


                internal b32 DEBUG_REQUESTED(debug_id ID)
                {
                    b32 Result = false;
                    
                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        Result = IsSelected(DebugState, ID)
                            || DebugIDsAreEqual(DebugState->HotInteraction.ID, ID);
                    }

                    return(Result);
                }

43:02
in the IsSelected function, we just go through all the entities that we have selected and check for ID match

                internal b32 IsSelected(debug_state *DebugState, debug_id ID)
                {
                    b32 Result = false;

                    for(u32 Index = 0;
                        Index < DebugState->SelectedIDCount;
                        ++Index)
                    {
                        if(DebugIDsAreEqual(ID, DebugState->SelectedID[Index]))
                        {
                            Result = true;
                            break;
                        }
                    }

                    return(Result);
                }





48:49
Casey also pointed out that when he defines these debug #define
if he changes the parameters in HANDMADE_INTERNAL, for example, changing 

                internal b32 DEBUG_HIGHLIGHTED(debug_id ID);
to 

                internal b32 DEBUG_HIGHLIGHTED(debug_id ID, v4 *Color);
    
he doest have to change it again in the else section 
becuz of the (...);   


                #if HANDMADE_INTERNAL
                internal void DEBUG_HIT(debug_id ID, r32 ZValue);
                internal b32 DEBUG_HIGHLIGHTED(debug_id ID, v4 *Color);
                internal b32 DEBUG_REQUESTED(debug_id ID);

                #else

                ...
                ...

                #define DEBUG_HIT(...)
                #define DEBUG_HIGHLIGHTED(...) 0
                #define DEBUG_REQUESTED(...) 0
                #endif 

50:53
Casey added the case for DebugInteraction_Select

                handmade_debug.cpp

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP, b32 AltUI)
                {
                    if(DebugState->HotInteraction.Type)
                    {
                        if(DebugState->HotInteraction.Type == DebugInteraction_AutoModifyVariable)
                        {
                            ...
                            ...

                        }

                        switch(DebugState->HotInteraction.Type)
                        {
                            case DebugInteraction_TearValue:
                            {
                                ...
                                ...
                            } break;

                            case DebugInteraction_Select:
                            {
                                // TODO(casey): Modifier keys or some way of doing multi-select?
                                if(0)
                                {
                                    ClearSelection(DebugState);
                                }
                                AddToSelection(DebugState, DebugState->HotInteraction.ID);
                            } break;                
                        }

                        DebugState->Interaction = DebugState->HotInteraction;
                    }
                    else
                    {
                        DebugState->Interaction.Type = DebugInteraction_NOP;
                    }
                }





the AddToSelection(); function is just adding the ID to a list. The list just contains a list of entities 
that is currenlty selected. (We may want to support multi select);


                handmade_debug.cpp

                internal void AddToSelection(debug_state *DebugState, debug_id ID)
                {
                    if((DebugState->SelectedIDCount < ArrayCount(DebugState->SelectedID)) &&
                       !IsSelected(DebugState, ID))
                    {
                        DebugState->SelectedID[DebugState->SelectedIDCount++] = ID;
                    }
                }

Q/A
1:00:25
about universal ids, what if somethings want to be identified in the same system, but doesnt have the fixed storage location 
or complex in some otherways that have uniqueness?

Casey says, the way he wrote debug_id, very specifically is 

                handmade_debug_interface.h 

                struct debug_id
                {
                    void *Value[2];
                };

that we have two pointers. the reason is specifically becuz you can almost always generate enough ids with enough "space"
lets say in one system, there is a storage with a certian for loop.
then you will have one pointer to indicate the storage
the other for the index of the for loop. 

meaning there are usually a way to identify something 
somtimes you may have to go as high as four 
                
                struct debug_id
                {
                    void *Value[4];
                };




