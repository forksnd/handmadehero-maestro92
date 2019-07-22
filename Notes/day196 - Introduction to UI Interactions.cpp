Handmade Hero Day 196 - Introduction to UI Interactions

Summary:

mentioned that in a UI flow, there are three components, the rendering code, the UI interaction code, and the game data code 
all three parts needs to be decoupled. [essentially MVC]

cleaned up and refactred our UI interaction code to be more decoupled 

introduced the concept of UI Interaction state.

Keyword:
UI, Code cleanup



5:00
mentions the concept of "IMGUI", Immeidate GUI

Casey mentioned that people often associate "IMGUI" with Single Execution Model 

what that means is that, lets take an example: 
lets say you have a checkbox  
             __
            |__| Foo 


what people usually write a Checkbox() function; is that inside this routine, people will check if the mouse inside the box.
if so, then it will toggle the check.

something like:

                void UITick()
                {
                    Checkbox();
                }

                void Checkbox()
                {
                    if(mouse inside and pressed)
                    {
                        ToggleCheck();
                        ...
                    }
                }


this is the most basic and straightforward way to do this 


6:12
there is a problem with this way of thinking about things: it makes it difficult to decouple 
the interactions, from the specific things that is on the screen.

for example, lets say I want the functionality of either 
1.  checking this checkbox or
2.  selecting this checkbox 
3.  or some other type of interaction...
             __
            |__| Foo 

then what that means is that every possible behaviour that i want to implement has to get stuffed inside the Checkbox(); routine.
what that means is that the person who is implementing the Checkbox(); routine needs to know all the possible interactions. 


7:43
Casey claims that he doesnt consider this approach to be IMGUI at all. He just thinks this is a 
very simple way of implementing Interaction processing. it has nothing to do if you are in immediate mode or not. 

Specifically the reason why he said that is becuz, the "Immediate mode" nature of IMGUI is only talking about that the GUI element is 
announced by the code. It is not talking about that the UI element had to be checkboxes. (meaning how the UI element is rendered);

Instead what could have been annouced here is just some generic thing.

for example:
I have a boolean, and I am not gonna annouce anything about how am I gonna process it as a UI element
I may be just putting boolean into a batch, that will contain information how to render this boolean.

then we can have a total separate set of code where people implemented different set of tools, which will scan through 
this batch and do things. 

The "Immediate mode" nature of IMGUI is just about the fact that the calling code doesnt have to save like a pointer.
it doesnt have to call a create(); function to create Widget. It just annouces the things that it has. 
[I didnt understand this part.... I assume he just means he wants the code to be as decoupled as possible. Basically MVC]

9:51
as an example, currently in our DEBUGEnd(); function 

this code is very Immeidate mode: we go through our debug data, and we spit out the data 
that the UI is interested in.  

essentially, we have three parts, 
1.  DrawDebugMainMenu(); is the rendering 
2.  the "Update Data For HotRecord" is processing the data as a result of the UI interaction 
3.  The "WasPressed()" portion is UI interaction. 


The UI interaction code is totally decoupled from the Debug Data Processing portion,
and also decoupled from the rendering(); portion. so all three parts are decoupled


this is the way Casey likes to structure his UI code

                handmade_debug.cpp

                internal void DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    ..........................................
                    ...... we get our HotRecord in here ......
                    DrawDebugMainMenu(DebugState, RenderGroup, MouseP);


                    .....................................................
                    ........... Update Data For HotRecord ...............
                    .....................................................


                    if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
                    {
                        if(HotRecord)
                        {
                            DebugState->ScopeToRecord = HotRecord;
                        }
                        else
                        {
                            DebugState->ScopeToRecord = 0;
                        }
                        RefreshCollation(DebugState);
                    }
                }

[is this just Model-View-Controller model?
    boolean is the data
    controller is the UI processing
    View is the rendering. All three code should be decoupled]



13:12
so now Casey wants to clean up the code a bit for the UI 

he mentions that he wants to make DrawDebugMainMenu(); return a piece of information describing 
what the mouse is over at this particular time. 


so in debug_state, Casey made debug_state have three variables Hot, InteractingWith and NextHot
these three are gonna form the basis of my UI system
                
                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...
                    
                    debug_interaction Interaction;
                    v2 LastMouseP;
                    debug_variable *Hot;
                    debug_variable *InteractingWith;
                    debug_variable *NextHot;

                    ...
                };



15:00
so now Caesy changed DEBUGEnd(); to have the following structure:

[so it seems like, when we render UI, its always a frame late?
    in DrawDebugMainMenu(); we are rendering, but also determining which element is the user_s mouse hovering on]

so DrawDebugMainMenu returns the UI element that the user_s mouse is hovering on.
Then the DEBUGInteract processes actual UI interaction. 


                handmade_debug.cpp

                internal void DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    TIMED_FUNCTION();
                    
                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        
                        ...
                        ...

                        DrawDebugMainMenu(DebugState, RenderGroup, MouseP);
                        DEBUGInteract(DebugState, Input, MouseP);        
                    }
                    ...
                }





15:36
So Casey adds the DEBUGInteract(); function. 
Essentially it takes in the UI Element that the user is hovering on, and then we do our UI interaction.

This is the Controller code in MVC, and in here we are mostly changing our UI Interaction Data 
meaning we are calling "DebugState->InteractingWith = 0", and "DebugState->InteractingWith = DebugState->Hot;"
based on whether we left clicked or right clicked

-   if our mouse is in a "released" state, then we are no longer interacting with any UI element 
    for which we call 

                DebugState->InteractingWith = 0;

-   if we our mouse in a "pressed" state, then we are interacting with DebugState->Hot;
                
                DebugState->InteractingWith = DebugState->Hot;

so what we are doing here right now is just updating our UI Interaction state. That is the first thing we are doing here

full code below:

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    ...

                    if(DebugState->Interaction)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        
                        if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DebugState->InteractingWith = 0;
                        }
                        ...
                        ...
                    }
                    else
                    {
                        DebugState->Hot = DebugState->NextHot;

                        if(Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount > 1)
                        {
                            // Handle Clicks inside a frame
                        }
                        else if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DebugState->InteractingWith = DebugState->Hot;
                        }

                    }
                }


23:05
then in the DrawDebugMainMenu(); Casey made a small change.

previously we have something like this:

                handmade_debug.cpp

                void DrawDebugMainMenu()
                {
                    ...
                    ...

                    while(Var)
                    {
                        ...

                        rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);        
                        if(IsInRectangle(Offset(TextBounds, TextP), MouseP))
                        {
                            ItemColor = V4(1, 1, 0, 1);
                            DebugState->HotVariable = Var;
                        }
                    }
                }


The DrawDebugMainMenu(); should only be a dumb function that takes data and renders it.
so intially, we are setting "DebugState->HotVariable = Var;", meaning we were changing the UI Interaction state.

no UI interaction state nor the game data should be altered at all in the rendering code. (Remember MVC!);
so we got rid of that in DrawDebugMainMenu(); 

The rendering code can only produce information that will be used later in the UI interaction code. 
(we are not changing the UI Interaction state, only just suggesting an UI element);

now we have the following code: 

                handmade_debug.cpp

                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    ...
                    ...

                    while(Var)
                    {
                        ...
                        DEBUGVariableToText(...);

                        rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);        
                        if(IsInRectangle(Offset(TextBounds, TextP), MouseP))
                        {
                            DebugState->NextHot = Var;
                        }

                        if(DebugState->Hot == Var)
                        {
                            ItemColor = V4(1, 1, 0, 1);
                        }

                        ...
                    }
                    ...
                }

as you can see, we just do a comparison of "if(DebugState->Hot == Var)" to render UI Interaction Data.



24:33
so currently in our DEBUGInteract(); function we have the following code 

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    v2 dMouseP = MouseP - DebugState->LastMouseP;

                    if(DebugState->Interaction)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        
                        if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
    end of a click      {
        ------------>      DebugState->InteractingWith = 0;
                        }
                    }
                    else
                    {
                        DebugState->Hot = DebugState->NextHot;

                        if(Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount > 1)
                        {
                            // process a full click in a frame
                        }
                        else if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
  begining of a click   {
        ------------>       DebugState->InteractingWith = DebugState->Hot;
                        }

                    }

                    DebugState->LastMouseP = MouseP;
                }


the thing is that, we have the logic for beginning of a click and logic for end of a click separately
What Casey wants is that if there is a whole click within a frame, Casey wants to do both UI responses

so Casey added two functions 
                
                handmade_debug.cpp

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    ...
                }

                internal void DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    ...
                }


25:17
so what Casey has now is 

-   notice that in the HalfTransitionCount > 1 case, which means we want to process a whole click in a frame, 
    we call BeginInteract(); and EndInteract(); together 

-   full code below:

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    v2 dMouseP = MouseP - DebugState->LastMouseP;

                    if(DebugState->Interaction)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        
                        if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DEBUGEndInteract();    
                        }
                    }
                    else
                    {
                        DebugState->Hot = DebugState->NextHot;

                        if(Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount > 1)
                        {
                            DEBUGBeginInteract();
                            DEBUGEndInteract();
                        }
                        else if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DEBUGBeginInteract();
                        }
                    }
                }

25:48
now Casey mentioned that we are doing a very simplified model of our user interaction
so we can do something where we count down the HalfTransitionCounts 

so the idea is that we will count how many HalfTransitions occured, and we will call the BeginInteract();
and EndInteract(); according to that.


-   then for the last if condition, we still have 1 x half transition left.
    if for that half transition, we EndedDown, we do another BeginInteract();
    [i still think that it should be TransitionIndex-=2. For every 2 x half transitions, we want to do a BeginInteract(); and EndInteract();?] 


                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {

                    if(DebugState->Interaction)
                    {
                        for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
                            TransitionIndex > 1;
                            --TransitionIndex)
                        {
                            DEBUGEndInteract(DebugState, Input, MouseP);
                            DEBUGBeginInteract(DebugState, Input, MouseP);
                        }

                        if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DEBUGEndInteract(DebugState, Input, MouseP);
                        }
                    }
                    else
                    {
                        DebugState->Hot = DebugState->NextHot;

                        for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
                            TransitionIndex > 1;
                            --TransitionIndex)
                        {
                            DEBUGBeginInteract(DebugState, Input, MouseP);
                            DEBUGEndInteract(DebugState, Input, MouseP);
                        }
                        
                        if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DEBUGBeginInteract(DebugState, Input, MouseP);
                        }
                    }
                }



30:20
Then Casey added the logic where if are dragging something in the "if(DebugState->Interaction)" case

For example, one thing we want to do is to change the values of our float through dragging with our mouse.


-   we change the amount based on the change in the mouse position. Hence we use dMouseP
    dMouseP just comes from the difference between the mouse position this frame vs the position from last frame. 

-   full code below:

                handmade_debug.cpp

                internal void DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    v2 dMouseP = MouseP - DebugState->LastMouseP;
                    
                    if(DebugState->Interaction)
                    {
                        // Mouse motion Logic
                        debug_variable *Var = DebugState->InteractingWith;
                        
                        // NOTE(casey): Mouse move interaction   
    changing value      switch(Var->Type)
    by dragging         {
    ------------>           case DebugVariableType_Real32:
                            {
                                Var->Real32 += 0.1f*dMouseP.y;
                            } break;
                        }

                        // Mouse Click Logic 
                        for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
                            TransitionIndex > 1;
                            --TransitionIndex)
                        {
                            DEBUGEndInteract(DebugState, Input, MouseP);
                            DEBUGBeginInteract(DebugState, Input, MouseP);
                        }

                        if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
                        {
                            DEBUGEndInteract(DebugState, Input, MouseP);
                        }
                    }
                    else
                    {
                        ...
                        ...
                    }
    -------->       DebugState->LastMouseP = MouseP;
                }



33:23
Casey coming back to finish up the DEBUGBeginInteract(); and DEBUGEndInteract(); function
in the DEBUGBeginInteract(); we mainly confirm the data that we want to interact

                handmade_debug.cpp

                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    DebugState->InteractingWith = DebugState->Hot;
                }


and we have the DEBUGEndInteract(); function
this is where we confirm the changing of the data. 
as you can see, we are actually toggling the Booleans here

then once we are done, we set DebugState->InteractingWith = 0;

                internal void DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    if(DebugState->Interaction != DebugInteraction_NOP)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        switch(Var->Type)
                        {
                            case DebugVariableType_Bool32:
                            {
                                Var->Bool32 = !Var->Bool32;
                            } break;
            
                            case DebugVariableType_Group:
                            {
                                Var->Group.Expanded = !Var->Group.Expanded;
                            } break;
                        }

                        WriteHandmadeConfig(DebugState);
                    }
                    
                    DebugState->InteractingWith = 0;
                }

so from a MVC stand point, in DEBUGBeginInteract(); we only changed our interaction state
in DEBUGEndInteract(); we changed the interaction state, as well as our game data.


36:03
So Casey Demonstrated an Edge case where if We click something outside of our 
menu, then hold the mouse then, drag the mouse ontop a menu Item, then the DebugState->Interaction
starts interacting with the item that you dragged ontop of. 

so what we need to do is to store more information in our interaction state.



36:57
So what Casey did is that he added another enum to describe what type of interaction we have having currently.
so Casey will make an enum, but he mentioned that you can also use a callback for this 

                handmade_debug.h

                enum debug_interaction
                {
                    DebugInteraction_None,

                    DebugInteraction_NOP,
                    
                    DebugInteraction_ToggleValue,
                    DebugInteraction_DragValue,
                    DebugInteraction_TearValue,
                };


and we store the interaction state in the debug_state 

                struct debug_state
                {
                    ...
                    debug_interaction Interaction;
                    ...
                };


so now in the DEBUGBeginInteract();, we will look at what type of UI element it is, and 
we will remember what type of UI Interaction we are doing 

-   so if we are interacting with a Bool32 or Group, we set the type of Interaction in our UI Interaction State to be 
    DebugInteraction_ToggleValue

-   if are dragging our floats, then we let our system know that the type of interaction is DebugInteraction_DragValue

-   otherwise, we just set it to DebugInteraction_NOP. In which we are interacting, but we arent interacting with anything valid.
    NOP means no Operations

                handmade_debug.cpp

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


then in the DEBUGInteract(); function we just verify it that if our interaction type is not DebugInteraction_NOP

notice also at the end, we reset the DebugState->Interaction Type to be DebugInteraction_None.

                handmade_debug.cpp

                internal void DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
   ------------->   if(DebugState->Interaction != DebugInteraction_NOP)
                    {
                        debug_variable *Var = DebugState->InteractingWith;
                        Assert(Var);
                        switch(DebugState->Interaction)
                        {
                            case DebugInteraction_ToggleValue:
                            {
                                switch(Var->Type)
                                {
                                    case DebugVariableType_Bool32:
                                    {
                                        Var->Bool32 = !Var->Bool32;
                                    } break;
                    
                                    case DebugVariableType_Group:
                                    {
                                        Var->Group.Expanded = !Var->Group.Expanded;
                                    } break;
                                }
                            } break;

                            case DebugInteraction_TearValue:
                            {
                            } break;
                        }

                        WriteHandmadeConfig(DebugState);
                    }
                    
                    DebugState->Interaction = DebugInteraction_None;
                    DebugState->InteractingWith = 0;
                }



49:17
So Casey introduced the concept of debug_variable_hierarchy.
mainly it contains the varaible "v2 UIP", which that contains the location 
that our debug UI is drawn. UIP means UI position

                handmade_debug.h

                struct debug_variable_group
                {
                    b32 Expanded;
                    debug_variable *FirstChild;    
                    debug_variable *LastChild;
                };

                struct debug_variable_hierarchy
                {
                    v2 UIP;
                    debug_variable *Group;
                };

so now debug_state has 

                handmade_debug.h

                struct debug_state
                {
                    ...
                    debug_variable_hierarchy Hierarchy;
                    ...
                };


then in our DEBUGStart(); function, we set the proper position for DebugState->Hierarchy.UIP

                internal void DEBUGStart(game_assets *Assets, u32 Width, u32 Height)
                {
                    TIMED_FUNCTION();

                    debug_state *DebugState = (debug_state *)DebugGlobalMemory->DebugStorage;
                    if(DebugState)
                    {
                        if(!DebugState->Initialized)
                        {
                            ...
                            ...
                        }
                        ...
                        ...

                        DebugState->Hierarchy.Group = DebugState->RootGroup;
                        DebugState->Hierarchy.UIP = V2(DebugState->LeftEdge, DebugState->AtY);
                    }
                }


52:56
then in the DrawDebugMainMenu(); our rendering will be aligned with our UIP instead of the 
DebugState->LeftEdge and DebugState->AtY variables 

                internal void
                DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    real32 AtX = DebugState->Hierarchy.UIP.x;
                    real32 AtY = DebugState->Hierarchy.UIP.y;

                    ...
                    ...
                }




