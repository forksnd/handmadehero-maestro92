Handmade Hero Day 199 - Reusing Debug Interactions

Summary:
Cleaned up the UI Interaction code
added BitmapDisplay on the debug UI system

Keyword:
UI 



5:21
fix a slight bug inside DEBUGBeginInteract();
in the case we take the suggested UI Interaction type, we forgot to set the debug_variable that we are interacting with 


                handmade_debug.cpp

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP, b32 AltUI)
                {
                    if(DebugState->HotInteraction)
                    {
                        DebugState->Interaction = DebugState->HotInteraction;
                        DebugState->InteractingWith = DebugState->Hot;
                    }
                    else
                    {
                        ...
                        ...
                    }
                }

7:10
Casey beginning to clean up the UI Interaction code.
the first thing he did is to bundle up all the debug_variable and debug_interaction_type information that was initially in debug_state 


so casey first renamed the enum debug_interaction to enum_interaction_type

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
                };

then he created a struct with debug_interaction

                struct debug_interaction
                {
                    debug_interaction_type Type;
                    union
                    {
                        void *Generic;
                        debug_variable *Var;
                        debug_variable_hierarchy *Hierarchy;
                        v2 *P;
                    };
                };



8:29
then in the debug_state, we just have 

"NextHotInteraction" is the one where our mouse is hovering on 
"HotInteraction" is the last known Interaction  [i dont see the point of HotInteraction]
"Interaction" is what the user is doing

                struct debug_state
                {
                    ...
                    debug_interaction Interaction;
                    debug_interaction HotInteraction;
                    debug_interaction NextHotInteraction;
                    ...
                    ...
                };


so things are a lot cleaner


14:33
Casey went on to clean up a lot of the UI code.




29:36
in day 198, we made Move and Resize work with Profile View. 
Now Casey wants to make the concept of position as a more generic case 

                handmade_debug.cpp

                struct debug_interaction
                {
                    debug_interaction_type Type;
                    union
                    {
                        ...
                        debug_variable *Var;
                        debug_variable_hierarchy *Hierarchy;
                        v2 *P;
                    };
                };

so Casey added a v2 *P in debug_interaction, which is a reference to the positions it wants to edit .


30:18
then in the DEBUGDrawMainMenu(); we set the position references of debug_interaction

                internal void
                DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    for(debug_variable_hierarchy *Hierarchy = DebugState->HierarchySentinel.Next;
                        Hierarchy != &DebugState->HierarchySentinel;
                        Hierarchy = Hierarchy->Next)
                    {
                        ...
                        while(Ref)
                        {
                            debug_variable *Var = Ref->Var;

                            ...
                            ...

                            switch(Var->Type)
                            {
                                case DebugVariableType_CounterThreadList:
                                {   
                                    ...
                                    ...

                                    debug_interaction SizeInteraction = {};
                                    SizeInteraction.Type = DebugInteraction_Resize;
                ---------------->   SizeInteraction.P = &Var->Profile.Dimension;

                                }
                            }
                        }
                
                        if(1)
                        {
                            debug_interaction MoveInteraction = {};
                            MoveInteraction.Type = DebugInteraction_Move;
                            MoveInteraction.P = &Hierarchy->UIP;

                            ...
                            ...
                        }
                    }
                }




finally in the DEBUGInteract(); function, we just edit the debug_variable position 
through the reference 

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    v2 dMouseP = MouseP - DebugState->LastMouseP;
                    

                    if(DebugState->Interaction.Type)
                    {
                        debug_variable *Var = DebugState->Interaction.Var;
                        debug_variable_hierarchy *Hierarchy = DebugState->Interaction.Hierarchy;
                        v2 *P = DebugState->Interaction.P;
                        
                        switch(DebugState->Interaction.Type)
                        {
                            ...
                            ...

                            case DebugInteraction_Resize:
                            {
                                *P += V2(dMouseP.x, -dMouseP.y);
                                P->x = Maximum(P->x, 10.0f);
                                P->y = Maximum(P->y, 10.0f);
                            } break;

                            case DebugInteraction_Move:
                            {
                                *P += V2(dMouseP.x, dMouseP.y);
                            } break;
                        }





35:03
Casey added also added a debug_bitmap_display as one of the variable types 

                struct debug_variable
                {
                    debug_variable_type Type;
                    char *Name;

                    union
                    {
                        b32 Bool32;
                        s32 Int32;
                        u32 UInt32;
                        r32 Real32;
                        v2 Vector2;
                        v3 Vector3;
                        v4 Vector4;
                        debug_variable_group Group;
                        debug_profile_settings Profile;
                        debug_bitmap_display BitmapDisplay;
                    };
                };

meaning, one of our debug_variable is a display of bitmaps 


                struct debug_bitmap_display
                {
                    bitmap_id ID;
                    v2 Dim;
                    b32 Alpha;
                };




36:42
so in the DEBUGDrawMainMenu(); function, we add the case for it 


                handmade_debug.h

                internal void DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    ...
                    ...
                    while(Ref)
                    {
                        ...
                        case DebugVariableType_BitmapDisplay:
                        {
                            ...
                            ...

                            PushRect(DebugState->RenderGroup, Bounds, 0.0f, V4(0, 0, 0, 1.0f));
                            PushBitmap(DebugState->RenderGroup, Var->BitmapDisplay.ID, BitmapScale, V3(MinCorner, 0.0f), V4(1, 1, 1, 1), 0.0f);

                        }
                        break;
                    }
                }


40:25
of course, we add the DEBUGAddVariable(); function for bitmap_id 

                handmade_debug_variables.h

                internal debug_variable_reference * DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, bitmap_id Value)
                {
                    debug_variable_reference *Ref = DEBUGAddVariable(Context, DebugVariableType_BitmapDisplay, Name);
                    Ref->Var->BitmapDisplay.ID = Value;
                    Ref->Var->BitmapDisplay.Dim = V2(25.0f, 25.0f);
                    Ref->Var->BitmapDisplay.Alpha = true;
                        
                    return(Ref);
                }

and now we now successfully render a bitmap in our profile debug view
