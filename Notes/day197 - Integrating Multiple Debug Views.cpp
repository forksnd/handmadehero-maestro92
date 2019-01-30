Handmade Hero Day 197 - Integrating Multiple Debug Views

Summary:
integrated the Profile log view into the debug_variable list view 

added support for new type of UI Interation: Profile View Resizing

changed a bit of the UI Interaction flow to support UI Elements that needs special UI Interactions 

the rendering code now gives the UI Interaction code the suggested debug_variable and the suggested UI InteractionType.
[I propose to refactor the rendering code so that the entire part that determmines the UI Interaction Type is 
 completely in there]

Keyword:
UI, debug system


3:34
Casey is considering to merge newly built "#define debug list view" with "our profiling log view"
that way we have a unified way of interacting with them



8:05
so Casey created another debug_variable_type. This is meant to be a "top level iterator" for our profiling log view

                handmade_debug.h

                enum debug_variable_type
                {
                    DebugVariableType_Bool32,
                    DebugVariableType_Int32,
                    DebugVariableType_UInt32,
                    DebugVariableType_Real32,
                    DebugVariableType_V2,
                    DebugVariableType_V3,
                    DebugVariableType_V4,

    ----------->    DebugVariableType_CounterThreadList,
                    
                    DebugVariableType_Group,
                };


9:05
Casey went on to refactor the DebugState initialization code

-   he pulled the debug_variable_definition_context creation out of DEBUGCreateVariables();

-   then casey attempted to add a DebugVariableType_CounterThreadList type variable.

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

                            InitializeArena(&DebugState->DebugArena, DebugGlobalMemory->DebugStorageSize - sizeof(debug_state), DebugState + 1);

                            debug_variable_definition_context Context = {};
                            Context.State = DebugState;
                            Context.Arena = &DebugState->DebugArena;
                            Context.Group = DEBUGBeginVariableGroup(&Context, "Root");

                            DEBUGCreateVariables(&Context);
                            debug_variable *ThreadList = DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "Profile by Thread");
                            
                            DebugState->RootGroup = Context.Group;
                            ...
                            ...
                        }
                    }

                    ...
                    ...
                }





13:24
then in the WriteHandmadeConfig(); function.
Previously, we were looping through our entire list of DebugVariables and writing them to our config file.
Casey mentiones that we dont need our profiling log output to be in our Config file

so Casey added the function, DEBUGShouldBeWritten();

                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState)
                {
                    ...
                    ...

                    debug_variable *Var = DebugState->RootGroup->Group.FirstChild;
                    while(Var)
                    {
                        if(DEBUGShouldBeWritten(Var->Type))
                        {
                            ............................................
                            ........... Write To Config File ...........
                            ............................................
                        }
                    }

                    ...
                }



14:22
here Casey writes the DEBUGShouldBeWritten(); function, which is just check if the type 
is DebugVariableType_CounterThreadList.

                handmade_debug.h

                inline b32 DEBUGShouldBeWritten(debug_variable_type Type)
                {
                    b32 Result = (Type != DebugVariableType_CounterThreadList);

                    return(Result);
                }


14:33
then in the DrawDebugMainMenu(); function, right now we are just looping through our DebugState->Hierarchy.
we are converting each debug_variable to text using the DEBUGVariableToText, then rendering each debug_variable 
using the DEBUGTextOutAt(); function.

so effectively, DrawDebugMainMenu(); is rendering each debug_variable as text.
now that we have DebugVariableType_CounterThreadList, Casey is adding a special case and rendering it differently.

-   as you can see below, we are just doing a switch statement.

                handmade_debug.cpp

                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    ...
                    ...

                    debug_variable *Var = DebugState->Hierarchy.Group->Group.FirstChild;
                    while(Var)
                    {
                        ...

                        rectangle2 Bounds = {};
                        switch(Var->Type)
                        {
                            case DebugVariableType_CounterThreadList:
                            {
                                ...
                                ...
                            } break;
                            
                            default:
                            {

                                DEBUGVariableToText(...);

                                ...
                                DEBUGTextOutAt();
                            }
                        }

                        ...
                        ...
                    }


16:54
So Casey now is taking his original profiling rendering code, into this DrawDebugMainMenu(); function

so originally the code is at DEBUGEnd(); Just to Recap, we were going through every debug_frame
for every debug_frame, we are going through every debug_frame_region, and we are calling a PushRect(); function 
on it. 

                handmade_debug.cpp

                internal void DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    ...
                    ...
                    if(DebugState->ProfileOn)
                    {
                        for(u32 FrameIndex = 0; FrameIndex < MaxFrame; ++FrameIndex)
                        {
                            debug_frame *Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);
                            ...
                            for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
                            {
                                debug_frame_region *Region = Frame->Regions + RegionIndex;

                                ...
                                ...

                                PushRect(RenderGroup, RegionRect, 0.0f, V4(Color, 1));
                            }
                        }
                    }
                }

so now Casey is transfering that code into the DebugVariableType_CounterThreadList case in DrawDebugMainMenu();

essentially, we first draw a big background, in which we compute the Bounds. 
then we we call DrawProfileIn();
                
                case DebugVariableType_CounterThreadList:
                {
                    v2 MinCorner = ....getMinCorner();......
                    v2 MaxCorner = ....getMaxCorner();......

                    Bounds = RectMinMax(MinCorner, MaxCorner);
                    DrawProfileIn(DebugState, Bounds, MouseP);

                }
                break;





20:01
the DrawProfileIn(); is exactly the same as what we had in DEBUGEnd();:
we go through each debug_frame, for each debug_frame, we go through each debug_frame_region, and we render each regions


                handmade_debug.cpp

                internal void DrawProfileIn(debug_state *DebugState, rectangle2 ProfileRect, v2 MouseP)
                {

                    ...
                    ...

                    for(u32 FrameIndex = 0; FrameIndex < MaxFrame; ++FrameIndex)
                    {
                        debug_frame *Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);
                        ...
                        for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
                        {
                            debug_frame_region *Region = Frame->Regions + RegionIndex;

                            ...
                            ...

                            PushRect(RenderGroup, RegionRect, 0.0f, V4(Color, 1));
                        }
                    }

                }


35:02
after much debugging, the profiling is finally being rendered.
Now Casey wants the profiling to be collapsable in the Hierarchy

So What Casey has to do is to make a group for it to live inside. 

-   so in the DEBUGStart(); initalization function,
we create a "Profile" debug variable group.
inside the group, we add the "By Thread" debug_variable, which is of type DebugVariableType_CounterThreadList.


                DEBUGBeginVariableGroup(&Context, "Profile");
                debug_variable *ThreadList =
                    DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "By Thread");
                DEBUGEndVariableGroup(&Context);
                

-   full code below:

                internal void DEBUGStart(game_assets *Assets, u32 Width, u32 Height)
                {
                    TIMED_FUNCTION();

                    debug_state *DebugState = (debug_state *)DebugGlobalMemory->DebugStorage;
                    if(DebugState)
                    {
                        if(!DebugState->Initialized)
                        {
                            DebugState->HighPriorityQueue = DebugGlobalMemory->HighPriorityQueue;
                        
                            InitializeArena(&DebugState->DebugArena, DebugGlobalMemory->DebugStorageSize - sizeof(debug_state), DebugState + 1);

                            debug_variable_definition_context Context = {};
                            Context.State = DebugState;
                            Context.Arena = &DebugState->DebugArena;
                            Context.Group = DEBUGBeginVariableGroup(&Context, "Root");

                            DEBUGCreateVariables(&Context);
                            DEBUGBeginVariableGroup(&Context, "Profile");
                            debug_variable *ThreadList =
                                DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "By Thread");
                            DEBUGEndVariableGroup(&Context);
                            
                            DebugState->RootGroup = Context.Group;


                            ...
                            ...
                        }

                    }
                }



38:41
Casey mentioned that we would want more stuff in the "Profile" debug_variable group.
so he made another group within the "Profile" group. 

[to make things more organized, I added some brackets and indentations]

-   as for the "By Thread" group and the "By Function" group. Remember in day 184, Casey mentioned that he wants 
    
    two ways to visualize the profiling data, 
    1.  the hierarchical view
    2.  the top slowest functions list view

    the "By Thread" group is for the No.1 view 
    the "By Functon" group is for the No.2 view


-   full code below:

                handmade_debug.cpp

                internal void DEBUGStart(game_assets *Assets, u32 Width, u32 Height)
                {
                    ...
                    ...

                    DEBUGBeginVariableGroup(&Context, "Debugging");

                    DEBUGCreateVariables(&Context);


                    DEBUGBeginVariableGroup(&Context, "Profile");
                    {
                        DEBUGBeginVariableGroup(&Context, "By Thread");
                        {
                            debug_variable *ThreadList = DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "");
                                                            ThreadList->Profile.Dimension = V2(1024.0f, 100.0f);
                        }
                        DEBUGEndVariableGroup(&Context);

                        DEBUGBeginVariableGroup(&Context, "By Function");
                        {
                            debug_variable *FunctionList =
                                DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "");
                            FunctionList->Profile.Dimension = V2(1024.0f, 200.0f);
                        }
                        DEBUGEndVariableGroup(&Context);
                    }
                    DEBUGEndVariableGroup(&Context);

                    ...
                    ...
                }


39:26
Casey now is complaining about slow MSVC (microsoft_s compiler); is. This came up cuz as he is messing with his debug ui menu,
we programtically rebuild and recompile our executable, and everytime he clicks open or close, there is still a slight pause.


39:45
the other thing Casye mentioned is that maybe we want to be able to resize these profile views. 
So Casey made it so that if this debug_variable represents a profile view, it will store its own debug_variable_settings 
data.


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
    --------------->    debug_profile_settings Profile;
                    };
                };


and Casey created the debug_profile_settings struct

                handmade_debug.h

                struct debug_profile_settings
                {
                    v2 Dimension;
                };



41:53
and now back to our DebugVariables creation code, we initalize the debug_profile_settings values 
for our debug_variables.


                DEBUGBeginVariableGroup(&Context, "Profile");
                        
                    DEBUGBeginVariableGroup(&Context, "By Thread");
                        debug_variable *ThreadList =
                            DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "");
                        ThreadList->Profile.Dimension = V2(1024.0f, 100.0f);
                    DEBUGEndVariableGroup(&Context);

                    DEBUGBeginVariableGroup(&Context, "By Function");
                        debug_variable *FunctionList =
                        DEBUGAddVariable(&Context, DebugVariableType_CounterThreadList, "");
                        FunctionList->Profile.Dimension = V2(1024.0f, 200.0f);
                    DEBUGEndVariableGroup(&Context);
               
                DEBUGEndVariableGroup(&Context);



so the idea is that, 
we have some debug_variable that stores boolean values,
        some debug_variable store vec4. 
Here we have "debug_variable *ThreadList" and "debug_variable *FunctionList",
these two will use their union to store debug_profile_settings data 




42:20
now that we are rendering the "By Thread" Profile view and "By Function" Profile view at different sizes 
(although the "By Function" profile view is exactly the same as the "By Thread" Profile view, for now);
Casey is thinking about adding a handle for the users to drag and resize the view



43:01
so now we go back to the DrawDebugMainMenu(); function, we draw a little handle 
in the DebugVariableType_CounterThreadList case

-   here we call PushRect(); just to draw a little handle

                void DrawDebugMainMenu()
                {

                    while(var)
                    {
                        ...
                        ...
                        case DebugVariableType_CounterThreadList:
                        {
                            v2 MinCorner = ....getMinCorner();......
                            v2 MaxCorner = ....getMaxCorner();......

                            Bounds = RectMinMax(MinCorner, MaxCorner);
                            DrawProfileIn(DebugState, Bounds, MouseP);

                            PushRect(DebugState->RenderGroup, SizeBox, 0.0f,
                                     (IsHot && (DebugState->HotInteraction == DebugInteraction_ResizeProfile)) ?
                                     V4(1, 1, 0, 1) : V4(1, 1, 1, 1));
                            ...
                            ...
                        }
                        break;
                        ...
                        ...
                    }
                }


45:18
so now Casey wants to add the functionality of dragging to resize.
This essentially is a new type of UI Interaction. 

so previously in our DEBUGBeginInteract(); function we have this notion that we determine the type of UI Interaction
depending on the type of the UI Element

                handmade_debug.h

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    if(DebugState->Hot)
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

as you can see, in our DEBUGBeginInteract(); function, we are doing a switch statement on the type of our 
UI Element, and we choose the DebugState->Interaction, which is our UI Interaction type. 

there are two schools of thought to solve this problem

1.  introduce a synthetic debug variable. 
    so something like a DebugVariableType_V2_Box_Dimensions.
    so you are dragging around a DebugVariableType_V2, but a special case V2, where it represents the dimensions of a box.

2.  instead of having DEBUGBeginInteract(); determing the UI Interaction Type based on the UI Element that it sees,
    it will get the UI Interaction Type based on whoever created that UI element says it should have.
       

so something like:

so the idea is that, the Rendering Code will now give the UI Interaction Code two pieces of infromation,
the suggested UI Element, as well as its suggested Interaction Type.

So pretty straightforward. Previously its only passing the suggested UI Element. Now we do two pieces of information. 

so if that UI Element doesnt have any special UI Interaction Type that the creator preferred, 
we just go with the regular case. 

[I am assuming the 2nd pass gives more flexibility? Essentially, a UI Element can now have different types of UI Interaction Type 
depending on different states and different situations. For example, if a certain debug_variable was in state 1, will have 
UI InteractionType A, then if this debug_variable is in state 2, it will have InteractionType B
so we can add crazy rules here.]



                handmade_debug.h

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    if(DebugState->Hot)
                    {
                        if(DebugState->HotInteraction)
                        {
    ------------>           DebugState->Interaction = DebugState->HotInteraction;
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









[## Confusion1 ##]

[Just want to do a bit of a summary here:

Is it just me, or this DEBUGBeginInteract(); UI code feels hacky?
    feels like there are too many if conditions in the UI code? I Hope Casey cleans this up

it seems to me that DEBUGBeginInteract(); has the logic of determining the UI InteractionType. 
the UI InteractionType should be determined by the UI Element.


the regular case for the algorithm is determined by the UI Element it is hovering on. However, there will be 
overrides for special cases, that is when this function receives a suggested UI Interaction Type.

if it does receive a special type of UI Interaction Type, it will just use that


so my question is, why not just move the entire algorithm to determine the UI InteractionType to the Rendering code?

you are already special casing it for case DebugVariableType_CounterThreadList, why not do that for all other types of 
debug_variable?

something like: 
                
                handmade_debug.h

                void DrawDebugMainMenu()
                {

                    while(var)
                    {
                        ...
                        ...
                        case DebugVariableType_CounterThreadList:
                        {
                            v2 MinCorner = ....getMinCorner();......
                            v2 MaxCorner = ....getMaxCorner();......

                            Bounds = RectMinMax(MinCorner, MaxCorner);
                            DrawProfileIn(DebugState, Bounds, MouseP);

                            PushRect(DebugState->RenderGroup, SizeBox, 0.0f,
                                     (IsHot && (DebugState->HotInteraction == DebugInteraction_ResizeProfile)) ?
                                     V4(1, 1, 0, 1) : V4(1, 1, 1, 1));
                            ...
                            ...
                        }
                        
                        default:
                        {
                            DEBUGVariableToText(...)
                            ...
                            ...
                            DEBUGTextOutAt(...);


                            case DebugVariableType_Bool32:
                            {
                                DebugState->NextHotInteraction = DebugInteraction_ToggleValue;
                            } break;

                            case DebugVariableType_Real32:
                            {
                                DebugState->NextHotInteraction = DebugInteraction_DragValue;
                            } break;

                            case DebugVariableType_Group:
                            {
                                DebugState->NextHotInteraction = DebugInteraction_ToggleValue;
                            } break;

                        }
                        ...
                        ...
                    }
                }

then in DEBUGBeginInteract(); we are literally just determining if its NOP or not 

it just feels alot cleaner this way

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    if(DebugState->Hot)
                    {
                        DebugState->Interaction = DebugState->HotInteraction;

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




I guess a big reason why i feel this current UI code by Casey is hacky is cuz there are 
too many if conditions all over the place. if you structure the code this way here,
you just got once place where you have a switch statement determing the UI Interaction Type.]









[## Confusion2 ##]

it seems to me that DEBUGBeginInteract(); has the logic of determining the UI InteractionType. 
the UI InteractionType should be determined by the UI Element.

i think the confusion comes from the fact that we dont have UI Element in our game, we only have debug_variable. 

previously, we have a one-to-one mapping of debug_variable to UI Element
each debug_variable will have a disntinct type of UI Interaction 

but the one-to-one mapping of debug_variable to UI Element is no longer true.

in this case, if we are hovering on the profile view, we want one type of interaction, but if our mouse is hovering on the resize box,
we want another type of interaction. 

however, both the profile view and resize box is represented by the debug_variable.

but the rendering code is only returning the debug_variable.


so i think ideally, to make the code cleaner, we would want something like:


     ___________________                                    
    |                   |     UI Element enum                ___________________________  
    |   rendering code  | ---------------------------->     |                           |   
    |___________________|     debug_varaible                |                           |
                                                            |                           |
                                                            |    DEBUGBeginInteract     |   --------> UI Interaction Type
                                                            |                           | 
                                                            |                           |
       User Input         ---------------------------->     |___________________________|
                                                            


that we we can have the code to be 

                void DEBUGDrawMainMenu()
                {
                    while(var)
                    {
                        ...
                        ...
                        case DebugVariableType_CounterThreadList:
                        {

                            ...
                            if(IsInRectangle(SizeBox, MouseP))
                            {
                                DebugState->NextUIElement = DebugUIElement_ResizeBox;
                                DebugState->NextHot = Var;
                            }
                            else if(IsInRectangle(Bounds, MouseP))
                            {
                                DebugState->NextUIElement = DebugUIElement_RegularBox;
                                DebugState->NextHot = Var;
                            }
                        }
                        break;
                        case DebugVariableType_Bool32:
                        {
                            DebugState->NextUIElement = DebugUIElement_ToggleBox;
                        }
                        ...
                        ...
                    }
                }


then in our DEBUGBeginInteract(); we will have 

                void DEBUGBeginInteract()
                {
                    case DebugUIElement_ToggleBox:
                    {
                        DebugState->NextHotInteraction = DebugInteraction_ToggleValue;
                    } break;

                    case DebugUIElement_DragBox:
                    {
                        DebugState->NextHotInteraction = DebugInteraction_DragValue;
                    } break;

                    case DebugUIElement_ToggleBox:
                    {
                        DebugState->NextHotInteraction = DebugInteraction_ToggleValue;
                    } break;                   
                }

i think this approach is worth a try.]




[## Confusion3 ##]
[while I mention in Confusion2 where a UI element will have a 1-to-1 mapping InteractionType, 
    i dont think I can make that assumption. 

lets say the debug_variable has a speical state, which changes the UI Interaction?

a UI InteractionType can only be determined by:

    debug_variable state + UI Element it is hovering on + input handling 


so to make a slight change in my DEBUGBeginInteract(); code, it will have to be 

                void DEBUGBeginInteract()
                {
                    case DebugUIElement_ToggleBox + debug_variable_state_1:
                    {
                        DebugState->NextHotInteraction = DebugInteraction_ToggleValue;
                    } break;

                    case DebugUIElement_DragBox + debug_variable_state_2:
                    {
                        DebugState->NextHotInteraction = DebugInteraction_DragValue;
                    } break;

                    case DebugUIElement_ToggleBox + + debug_variable_state_3:
                    {
                        DebugState->NextHotInteraction = DebugInteraction_ToggleValue;
                    } break;                   
                }

the graph 

     ___________________                                    
    |                   |     UI Element enum                ___________________________  
    |   rendering code  | ---------------------------->     |                           |   
    |___________________|     debug_varaible                |                           |
                                                            |                           |
                                                            |    DEBUGBeginInteract     |   --------> UI Interaction Type
                                                            |                           | 
                                                            |                           |
       User Input         ---------------------------->     |___________________________|
                                                            
still holds.]








47:11
so Casey added that variable in the debug_state.

[If I were him, i would bundle these two together]

                struct debug_state
                {
                    ...
                    ...

                    debug_interaction HotInteraction;
                    debug_variable* Hot;
                
                    ...
                };


49:22
then in the DrawDebugMainMenu();, we initalize the suggested debug_variable and the UI Interaction type 
-   so in the DebugVariableType_CounterThreadList case, the suggested UI InteractionType depends on whether you are mouse is 
    hovering on the ResizeBox.

-   in the default case, DebugState->NextHotInteraction = DebugInteraction_None; is always none.


                handmade_debug.cpp

                void DrawDebugMainMenu()
                {

                    while(var)
                    {
                        ...
                        ...
                        case DebugVariableType_CounterThreadList:
                        {
                            v2 MinCorner = ....getMinCorner();......
                            v2 MaxCorner = ....getMaxCorner();......

                            Bounds = RectMinMax(MinCorner, MaxCorner);
                            DrawProfileIn(DebugState, Bounds, MouseP);

                            PushRect(DebugState->RenderGroup, SizeBox, 0.0f,
                                     (IsHot && (DebugState->HotInteraction == DebugInteraction_ResizeProfile)) ?
                                     V4(1, 1, 0, 1) : V4(1, 1, 1, 1));

                            if(IsInRectangle(SizeBox, MouseP))
                            {
                                DebugState->NextHotInteraction = DebugInteraction_ResizeProfile;
                                DebugState->NextHot = Var;
                            }
                            else if(IsInRectangle(Bounds, MouseP))
                            {
                                DebugState->NextHotInteraction = DebugInteraction_None;
                                DebugState->NextHot = Var;
                            }

                        }
                        break;
                        
                        default:
                        {
                            DEBUGVariableToText(...)
                            ...
                            ...
                            DEBUGTextOutAt(...);

                            if(IsInRectangle(Bounds, MouseP))
                            {
                                DebugState->NextHotInteraction = DebugInteraction_None;
                                DebugState->NextHot = Var;
                            }

                        ...
                        ...
                    }
                }




48:31
added this new type of debug_interaction

                handmade_debug.h

                enum debug_interaction
                {
                    DebugInteraction_None,

                    DebugInteraction_NOP,
                    
                    DebugInteraction_ToggleValue,
                    DebugInteraction_DragValue,
                    DebugInteraction_TearValue,

    ------------>   DebugInteraction_ResizeProfile,
                };



52:49
Casey then adds the Processing for the DebugInteraction_ResizeProfile Interaction Type




                if(DebugState->Interaction)
                {
                    debug_variable *Var = DebugState->InteractingWith;
                    
                    // NOTE(casey): Mouse move interaction
                    switch(DebugState->Interaction)
                    {
                        case DebugInteraction_DragValue:
                        {
                            switch(Var->Type)
                            {
                                case DebugVariableType_Real32:
                                {
                                    Var->Real32 += 0.1f*dMouseP.y;
                                } break;
                            }
                        } break;

    --------------->    case DebugInteraction_ResizeProfile:
                        {
                            Var->Profile.Dimension += V2(dMouseP.x, -dMouseP.y);
                            Var->Profile.Dimension.x = Maximum(Var->Profile.Dimension.x, 10.0f);
                            Var->Profile.Dimension.y = Maximum(Var->Profile.Dimension.y, 10.0f);
                        } break;
                    }
                }