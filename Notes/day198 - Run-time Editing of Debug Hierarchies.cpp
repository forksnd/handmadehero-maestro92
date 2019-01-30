Handmade Hero Day 198 - Run-time Editing of Debug Hierarchies

Summary:
added the concept of debug_variable_reference. Have all the debug_variable, debug_variable_group and debug_variable_hierarchy
store debug_variable_reference instead of debug_variable.

this way we can have multiple Hierarchy on the debug UI.

added UI Interaction type DebugInteraction_TearValue, where we can copy and drag a debug_variable into a separate Hierarchy, 
added UI Interaction type DebugInteraction_MoveHierarchy, where we can move Hierarchy.

Keyword:
UI


3:49
Casey mentioned that he wants to be able to drag the "By Thread" Profile View out to anywhere on the screen


4:50
so Casey mentioned that currently we have our debug_variable like below:

                handmade_debug.h

                struct debug_variable
                {
                    debug_variable_type Type;
                    char *Name;
                    debug_variable *Next;
                    debug_variable *Parent;

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
                    };
                };

the debug_variable itself stores the data that we are editing. the problem with that is it is hard to put the debug_variable
in more than one place. What we much rather do is have this to be indirected in someway.

we would like the debug_variable to be refeerenceable in more than one places simultaenously.


5:25
doubly linked in-directed list just arent that common in games 

so Casey created the debug_variable_reference struct 


                handmade_debug.h

                struct debug_variable_reference
                {
                    debug_variable *Var;
                    debug_variable_reference *Next;
                    debug_variable_reference *Parent;
                };

                struct debug_variable
                {
                    ...
                    ...    
                };

now debug_variable_group will all have references instead of actual variables 


                struct debug_variable_group
                {
                    b32 Expanded;
                    debug_variable_reference *FirstChild;    
                    debug_variable_reference *LastChild;
                };

8:05
then in DEBUGAddVariable(); function, we want to be able to make both the debug_variable and the debug_variable_reference

                handmade_debug.cpp

                internal debug_variable_reference * DEBUGAddVariable(debug_variable_definition_context *Context, debug_variable_type Type, char *Name)
                {    
                    debug_variable *Var = DEBUGAddUnreferencedVariable(Context->State, Type, Name);
                    debug_variable_reference *Ref = DEBUGAddVariableReference(Context, Var);
                    
                    return(Ref);
                }

then Casey wrote the two functions, one just creates the debug_variable, 
                
                internal debug_variable * DEBUGAddUnreferencedVariable(debug_state *State, debug_variable_type Type, char *Name)
                {
                    debug_variable *Var = PushStruct(&State->DebugArena, debug_variable);
                    Var->Type = Type;
                    Var->Name = (char *)PushCopy(&State->DebugArena, StringLength(Name) + 1, Name);
                    
                    return(Var);
                }

the other, the reference.

                internal debug_variable_reference * DEBUGAddVariableReference(debug_state *State, debug_variable_reference *GroupRef, debug_variable *Var)
                {
                    debug_variable_reference *Ref = PushStruct(&State->DebugArena, debug_variable_reference);
                    Ref->Var = Var;
                    Ref->Next = 0;

                    Ref->Parent = GroupRef;
                    debug_variable *Group = (Ref->Parent) ? Ref->Parent->Var : 0; 
                    if(Group)
                    {
                        if(Group->Group.LastChild)
                        {
                            Group->Group.LastChild = Group->Group.LastChild->Next = Ref;
                        }
                        else
                        {
                            Group->Group.LastChild = Group->Group.FirstChild = Ref;
                        }
                    }

                    return(Ref);
                }


14:19
Casey proceeds to change debug_variable to debug_variable_reference everywhere: 
then in the debug_state.RootGroup, we change it to a debug_variable_reference as well 

                handmade_debug.cpp

                struct debug_state
                {
                    ...
                    ...
                    debug_variable_reference * RootGroup;
                    ...
                    ...
                };


inside of the debug_variable_hierarchy, we also change it to debug_variable_reference

                struct debug_variable_hierarchy
                {
                    v2 UIP;
                    debug_variable_reference *Group;

                    debug_variable_hierarchy *Next;
                    debug_variable_hierarchy *Prev;
                };

                ...
                ...


16:49
Now Casey will test the referencing by adding a debug_variable_reference to two different groups 

so in the DEBUGCreateVariables(); variable, we add the UseDebugCamRef debug_variable to 2 different groups 

                handmade_debug_variables.h

                internal void DEBUGCreateVariables(debug_variable_definition_context *Context)
                {
                // TODO(casey): Parameterize the fountain?

                    debug_variable_reference *UseDebugCamRef = 0;
                    
                #define DEBUG_VARIABLE_LISTING(Name) DEBUGAddVariable(Context, #Name, DEBUGUI_##Name)

                    ...
                    ...
                    
                    DEBUGBeginVariableGroup(Context, "Renderer");
                    {        
                        DEBUG_VARIABLE_LISTING(TestWeirdDrawBufferSize);
                        DEBUG_VARIABLE_LISTING(ShowLightingSamples);

                        DEBUGBeginVariableGroup(Context, "Camera");
                        {
                            UseDebugCamRef = DEBUG_VARIABLE_LISTING(UseDebugCamera);
                            DEBUG_VARIABLE_LISTING(DebugCameraDistance);
                            DEBUG_VARIABLE_LISTING(UseRoomBasedCamera);
                        }
                        DEBUGEndVariableGroup(Context);

                        DEBUGEndVariableGroup(Context);
                    }

                    ...
                    ...

                    DEBUGAddVariableReference(Context, UseDebugCamRef->Var);

                #undef DEBUG_VARIABLE_LISTING
                }


20:36
now that Casey wants to split off a menu, all we have to do is to make another debug_variable_hierarchy

                handmade_debug.h

                struct debug_variable_hierarchy
                {
                    v2 UIP;
                    debug_variable_reference* Group;
                };

the debug_variable_hierarchy is what controls what is drawn, and right now we only have one of them.
so what we want is to have as many debug_variable_hierarchy as possible. 

Casey created a doubly linked list as the container for all of its Hierarchies;


                handmade_debug.cpp

                struct debug_state
                {
                    ...
                    debug_variable_hierarchy HierarchySentinel;
                    ...
                };


22:09

so now in the DEBUGDrawMainMenu(); we would loop through a list of Hierarchy
                
                handmade_debug.cpp

                internal void DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    for(debug_variable_hierarchy *Hierarchy = DebugState->HierarchySentinel.Next;
                        Hierarchy != &DebugState->HierarchySentinel;
                        Hierarchy = Hierarchy->Next)
                    {
                        .................................................
                        ........... Render A Hierarchy ..................
                        .................................................
                    }
                }


Of course Casey had to change the debug_variable_hierarchy to a doubly linked list node

                handmade_debug.h

                struct debug_variable_hierarchy
                {
                    v2 UIP;
                    debug_variable_reference *Group;

                    debug_variable_hierarchy *Next;
                    debug_variable_hierarchy *Prev;
                };



24:34
to make it easier to add multiple Hierarchies, Casey created this AddHierarchy(); function
essentially, we pass in a debug_variable_reference group, and we create a Hierarchy out of that debug_variable_reference group

                handmade_debug.cpp

                internal debug_variable_hierarchy * AddHierarchy(debug_state *DebugState, debug_variable_reference *Group, v2 AtP)
                {
                    debug_variable_hierarchy *Hierarchy = PushStruct(&DebugState->DebugArena, debug_variable_hierarchy);
                    
                    Hierarchy->UIP = AtP;
                    Hierarchy->Group = Group;
                    Hierarchy->Next = DebugState->HierarchySentinel.Next;
                    Hierarchy->Prev = &DebugState->HierarchySentinel;

                    Hierarchy->Next->Prev = Hierarchy;
                    Hierarchy->Prev->Next = Hierarchy;

                    return(Hierarchy);
                }



32:10
so for our main debug_variable initalization code, once we finish creating the RootGroup, we make DebugState->RootGroup 
into a Hierarchy, by calling AddHierarchy();

                handmade_debug.cpp

                internal void DEBUGStart(game_assets *Assets, u32 Width, u32 Height)
                {
                    TIMED_FUNCTION();

                    debug_state *DebugState = (debug_state *)DebugGlobalMemory->DebugStorage;
                    if(DebugState)
                    {
                        if(!DebugState->Initialized)
                        {
                            ...

                            DebugState->RootGroup = Context.Group;
                            
                            ...
                            ...

                            AddHierarchy(DebugState, DebugState->RootGroup, V2(-0.5f*Width, 0.5f*Height));
                        }
                    }
                }


32:58
Now Casey would want to introduce a new type of UI Interaction Type, which is to enable tearing debug_variable_group off.



33:26
so recall from previous days, Casey has already added the DebugInteraction_TearValue interaction type.

                handmade_debug.h

                enum debug_interaction
                {
                    DebugInteraction_None,

                    DebugInteraction_NOP,
                    
                    DebugInteraction_ToggleValue,
                    DebugInteraction_DragValue,
                    DebugInteraction_TearValue,

                    DebugInteraction_ResizeProfile,
                    DebugInteraction_MoveHierarchy,
                };




34:15 
Casey wants to make it so that every debug_variable can have the functionality to be teared off. 


36:58
he also assumes that if the user has the right mouse key held down, then we would be switching the UI Interaction type 
to the TearOff mode 
               
as you can see, we first check if the PlatformMouseButton_Right.EndedDown. 

then we pass it into the DEBUGBeginInteract(); function

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {

                    ...
                    ...
                    
                    b32 AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
                    // NOTE(casey): Click interaction
                    for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
                        TransitionIndex > 1;
                        --TransitionIndex)
                    {
                        DEBUGEndInteract(DebugState, Input, MouseP);
                        DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
                    }

                    ...
                    ...
                }


[in day 197, I was suggesting to move all the logic that determines what the suggested UI Interaction type to 
    the Rendering case

here comes a more complicated scenario, where we have a different UI Interaction Type when another key is held down. 
that makes me more prone to Casey_s current code organization. Since whether the PlatformMouseButton_Right.EndedDown 
should ever exist in the rendering code.

so the UI Interaction Type will be determined by:

1.  mouse position on the UI element 
2.  the state of the debug_variable, 
3.  and what other key inputs the user has.

so part1 lives inside the rendering code
part2 and part3 lives in the UI Interaction code. 

I suppose thats our structure?]


so in the DEBUGBeginInteract(); function, if there was a special UI Interaction type, we would choose the HotInteraction Type.
then if the "AltUI" flag is on, we would go for DebugInteraction_TearValue,

otherwise, we just do are regular UI Interaction Type. 


[so here we see complicated rules, with one overriding another, or others having priorities.
I suppose UI Interaction rules is expected to be this complicated?

the model that I am used to is to have a separate function where we loop through all UI elements and see which UI element 
the mouse is hovering on. But that will require us to store a list of UI elements.

whereas here, Casey isnt doing that, all this "Mouse hovering code" lives inside the rendering code]


                handmade_debug.cpp

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP, b32 AltUI)
                {
                    if(DebugState->HotInteraction)
                    {
                        DebugState->Interaction = DebugState->HotInteraction;
                    }
                    else
                    {
                        if(DebugState->Hot)
                        {
                            if(AltUI)
                            {
                                DebugState->Interaction = DebugInteraction_TearValue;
                            }
                            else
                            {
                                switch(DebugState->Hot->Type)
                                {
                                    case DebugVariableType_Bool32:
                                    {
                                        DebugState->Interaction = DebugInteraction_ToggleValue;
                                    } break;

                                    case DebugVariableType_Real32:
                                    {
                                        DebugState->Interaction = DebugInteraction_DragValue;
                                    } break;

                                    case DebugVariableType_Group:
                                    {
                                        DebugState->Interaction = DebugInteraction_ToggleValue;
                                    } break;
                                }
                            }

                            if(DebugState->Interaction)
                            {
                                DebugState->InteractingWith = DebugState->Hot;
                            }
                        }
                        else
                        {
                            DebugState->Interaction = DebugInteraction_NOP;
                        }
                    }
                }


38:53
now Casey goes on to implement DebugInteraction_TearValue
then in our DEBUGInteract(); code, we add the case for DebugInteraction_TearValue. 

-   what we are doing here is that, we want to create a new Hierarchy for the debug_varaible that was "Teared Off"
    
    so we first create a RootGroup, named "NewUserGroup"

                debug_variable_reference *RootGroup = DEBUGAddRootGroup(DebugState, "NewUserGroup");


    then we copy the source debug_variable, which is "DebugState->InteractingWith", onto our new debug_variable_group

                DEBUGAddVariableReference(DebugState, RootGroup, DebugState->InteractingWith);


    finally, we create a Hierarchy out of that new "NewUserGroup"

                DebugState->DraggingHierarchy = AddHierarchy(DebugState, RootGroup, V2(0, 0));


-   lastly, we update the position of this Hierarchy by updating with the mouse position 

                DebugState->DraggingHierarchy->UIP = MouseP;



-   full code below:

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    v2 dMouseP = MouseP - DebugState->LastMouseP;
                    
                    if(DebugState->Interaction)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        
                        // NOTE(casey): Mouse move interaction
                        switch(DebugState->Interaction)
                        {
                            case DebugInteraction_DragValue:
                            {
                                ...
                            } break;

                            case DebugInteraction_ResizeProfile:
                            {
                                Var->Profile.Dimension += V2(dMouseP.x, -dMouseP.y);
                                Var->Profile.Dimension.x = Maximum(Var->Profile.Dimension.x, 10.0f);
                                Var->Profile.Dimension.y = Maximum(Var->Profile.Dimension.y, 10.0f);
                            } break;

                            ...
                            ...

        --------------->    case DebugInteraction_TearValue:
                            {
                                if(!DebugState->DraggingHierarchy)
                                {
                                    debug_variable_reference *RootGroup = DEBUGAddRootGroup(DebugState, "NewUserGroup");
                                    DEBUGAddVariableReference(DebugState, RootGroup, DebugState->InteractingWith);
                                    DebugState->DraggingHierarchy = AddHierarchy(DebugState, RootGroup, V2(0, 0));
                                }

                                DebugState->DraggingHierarchy->UIP = MouseP;
                            } break;
                        }
                        ...
                        ...
                    }
                }



[like what I said in day 197, I still prefer this approach:

     ___________________                                    
    |                   |     UI Element enum                ___________________________  
    |   rendering code  | ---------------------------->     |                           |   
    |___________________|     debug_varaible                |                           |
                                                            |                           |
                                                            |    DEBUGBeginInteract     |   --------> UI Interaction Type
                                                            |                           | 
                                                            |                           |
       User Input         ---------------------------->     |___________________________|
                                                            

the rendering code out puts the debug_variable and a enum that represents where the mouse is hovering 


then we take that both information + any user input to determine the UI Interaction Type

then all the complicated UI Interaction rules can be all placed in DEBUGBeginInteract(); instead of
spread across both the rendering code and the DEBUGBeginInteract(); code, which is soo confusing...]





55:15
so now, as you can see in the video, you can create multiple copies of debug_variable_hierarchy.
what Casey wants to do is to for each Hierarchy to have their own expand/contract state.

for example, assume we have the Particles Hierarchy on the left, and another one which we "teared off"
whether each one is expanded or contract, we want them to have separate states. 



     _______________________________________________________________________________
    |                                                                               |
    |   Particles: (expanded)                                                       |
    |       ParticleTest:   True                        Particles: (contract)       |
    |       ParticleGrid:   true                                                    |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |_______________________________________________________________________________|


but this will be addressed next week. 


56:03
so the last thing Casey wants to do is to be able to move Hierarchy. 
currently, once you make a duplicate Hierarchy (with the same reference);, you can only move it 
when you create and drag it the first time.

Casey would like it so that you can move the Hierarchy anytime you want. 

so Casey added introduced anoterh UI Interaction Type 


                handmade_debug.cpp

                enum debug_interaction
                {
                    DebugInteraction_None,

                    DebugInteraction_NOP,
                    
                    DebugInteraction_ToggleValue,
                    DebugInteraction_DragValue,
                    DebugInteraction_TearValue,

                    DebugInteraction_ResizeProfile,
    ------------>   DebugInteraction_MoveHierarchy,
                };




56:33
and in the DEBUGINteract(); function, we add the case for that 

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    v2 dMouseP = MouseP - DebugState->LastMouseP;
                    
                    if(DebugState->Interaction)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        
                        // NOTE(casey): Mouse move interaction
                        switch(DebugState->Interaction)
                        {
                            case DebugInteraction_DragValue:
                            {
                                ...
                            } break;

                            case DebugInteraction_ResizeProfile:
                            {
                                ...
                            } break;

        --------------->    case DebugInteraction_MoveHierarchy:
                            {
                                DebugState->DraggingHierarchy->UIP += V2(dMouseP.x, dMouseP.y);
                            } break;

                            case DebugInteraction_TearValue:
                            {
                                if(!DebugState->DraggingHierarchy)
                                {
                                    debug_variable_reference *RootGroup = DEBUGAddRootGroup(DebugState, "NewUserGroup");
                                    DEBUGAddVariableReference(DebugState, RootGroup, DebugState->InteractingWith);
                                    DebugState->DraggingHierarchy = AddHierarchy(DebugState, RootGroup, V2(0, 0));
                                }

                                DebugState->DraggingHierarchy->UIP = MouseP;
                            } break;
                        }



57:35
so just as a hack, Casey added a small box, that people can drag and move.

                handmade_debug.cpp

                internal void DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    for(debug_variable_hierarchy *Hierarchy = DebugState->HierarchySentinel.Next;
                        Hierarchy != &DebugState->HierarchySentinel;
                        Hierarchy = Hierarchy->Next)
                    {
                        ...
                        ...

                        if(1)
                        {
                            rectangle2 MoveBox = RectCenterHalfDim(Hierarchy->UIP - V2(4.0f, 4.0f), V2(4.0f, 4.0f));
                            PushRect(DebugState->RenderGroup, MoveBox, 0.0f,
                                     V4(1, 1, 1, 1));

                            if(IsInRectangle(MoveBox, MouseP))
                            {
                                DebugState->NextHotInteraction = DebugInteraction_MoveHierarchy;
                                DebugState->NextHotHierarchy = Hierarchy;
                            }
                        }
                    }

[again, I think passing an UI element enum + debug_variable is worth a try]


Q/A
1:16:48
someone mentioned why is there so much lag when you drag a Hierarchy?

cuz our collation is still expensive. when the collation restarts, we lag. The dragging Hierarchy itself 
is almost instantaenous.
