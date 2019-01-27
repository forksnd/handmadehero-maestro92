Handmade Hero Day 195 - Implementing an Interactive Tree View

Summary:
rendered our debug variable hiearchy in the DrawDebugMainMenu(); function.

added more debug_variable_type types. Whereas previously we only had DebugVariableType_Bool32

however, right now we can still only just toggle the DebugVariableType_Bool32 values

Keyword:
UI



6:24
now in DrawDebugMainMenu();, we iterate through all of our debug variables and render it as menu items 
its essentially iterating through the entire tree structure. pretty straight-forwards
[note that we are just rendering as a list of items, we are no longer doing a radial menu]

                handmade_debug.cpp

                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    ...
                    ...

                    DebugState->HotVariable = 0;
                    
                    int Depth = 0;
                    debug_variable *Var = DebugState->RootGroup->Group.FirstChild;
                    while(Var)
                    {
                        ...
                        ...

                        DEBUGTextOutAt(TextP, Text, ItemColor);
                        AtY -= LineAdvance*DebugState->FontScale;

                        if((Var->Type == DebugVariableType_Group) && Var->Group.Expanded)
                        {
                            Var = Var->Group.FirstChild;
                            ++Depth;
                        }
                        else
                        {
                            while(Var)
                            {
                                if(Var->Next)
                                {
                                    Var = Var->Next;
                                    break;
                                }
                                else
                                {
                                    Var = Var->Parent;
                                    --Depth;
                                }
                            }
                        }
                    }    

                    DebugState->AtY = AtY;
                }




17:17
adding more logic in DEBUGEnd(); for selecting the menu item 

so when we left click on a boolean, we would toggle it, 
if we click on a group, we would expand/contract the debug_variable list

                internal void
                DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {

                    ...
                    ...

                    if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
                    {
                        if(DebugState->HotVariable)
                        {
                            debug_variable *Var = DebugState->HotVariable;
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

                                InvalidDefaultCase;
                            }

                            WriteHandmadeConfig(DebugState);
                        }
                    }

                    ...
                    ...
                }


20:49
while now we rendering our debug view 
Casey mentioned that we have a bug in which our debug string are stored in the dll,
which means, when we recompile, our string pointer is no longer valid.

previously, whenever we call 

                DEBUG_VARIABLE_LISTING(GroundChunkOutlines);

that becomes 

                DEBUGAddVariable(&Context, #Name, "DEBUGUI_GroundChunkOutlines")

that will call this 

                internal debug_variable * DEBUGAddVariable(debug_variable_definition_context *Context, debug_variable_type Type, char *Name)
                {    
                    debug_variable *Var = PushStruct(Context->Arena, debug_variable);
                    Var->Type = Type;
                    Var->Name = Name;
                    Var->Next = 0;

                    ...
                    ...
                }


[literals are placed into the static initalization portion of the executable]
https://stackoverflow.com/questions/18321822/what-is-the-difference-between-hard-coding-and-passing-in-arguments-in-regard


so anytime you recompile, that memory will become invalid. 

so what need to do is that when we build our debug_variable table, we need to copy these strings into our debug_memory,
then our debug_variable->Name will be reading the string values from the debug_memory



22:09
Casey defined the PushCopy function

                handmade.h

                ...
                ...

                #define PushCopy(Arena, Size, Source, ...) Copy(Size, Source, PushSize_(Arena, Size, ## __VA_ARGS__))

                ...
                ...



24:04
so now the debug_variable->Name will be getting its name from the debug_memory of our program. This way when we reload
our executable, strings memory arent messed up 

                handmade_debug.cpp

                internal debug_variable * DEBUGAddVariable(debug_variable_definition_context *Context, debug_variable_type Type, char *Name)
                {    
                    debug_variable *Var = PushStruct(Context->Arena, debug_variable);
                    Var->Type = Type;
                    Var->Name = (char *)PushCopy(Context->Arena, StringLength(Name) + 1, Name);
                    Var->Next = 0;

                    debug_variable *Group = Context->Group;
                    Var->Parent = Group;
                    
                    if(Group)
                    {
                        if(Group->Group.LastChild)
                        {
                            Group->Group.LastChild = Group->Group.LastChild->Next = Var;
                        }
                        else
                        {
                            Group->Group.LastChild = Group->Group.FirstChild = Var;
                        }
                    }
                    
                    return(Var);
                }

32:30
now Casey wants to add other types of variables

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
                    
                    DebugVariableType_Group,
                };

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
                    };
                };



34:10
then in the WriteHandmadeConfig();
we writeout the variables depending on the type 

-   notice that we pass in the u32 Flags value 

                DEBUGVarToText_AddDebugUI|
                DEBUGVarToText_AddName|
                DEBUGVarToText_LineFeedEnd|
                DEBUGVarToText_FloatSuffix

    that is to indicate what are flags that we want to output. 


-   full code below: 

                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState)
                {
                    ...
                    ...

                    int Depth = 0;
                    debug_variable *Var = DebugState->RootGroup->Group.FirstChild;
                    while(Var)
                    {        
                        ...
                        ...

                        if(Var->Type == DebugVariableType_Group)
                        {
                            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                              "// ");
                        }
                        At += DEBUGVariableToText(At, End, Var,
                                                  DEBUGVarToText_AddDebugUI|
                                                  DEBUGVarToText_AddName|
                                                  DEBUGVarToText_LineFeedEnd|
                                                  DEBUGVarToText_FloatSuffix);
                        ...
                        ...
                    }
                }



42:49
so in the DEBUGVariableToText(); function is literally a giant switch + if statement
                                
-   depending on the flags value that we pass in, we output things accordingly. 
    For example, we passed in the DEBUGVarToText_FloatSuffix value. 
    then when we get to the DebugVariableType_Real32 case, we check if we want to output the 
    'f' FloatSuffix

                case DebugVariableType_Real32:                
                {
                    At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                      "%f", Var->Real32);
                    if(Flags & DEBUGVarToText_FloatSuffix)
                    {
                        *At++ = 'f';
                    }
                } break;


-   same thing with DEBUGVarToText_PrettyBools. For that we can either 
    print out 1 or 0, and just print out true or false 

                case DebugVariableType_Bool32:
                {
                    if(Flags & DEBUGVarToText_PrettyBools)
                    {
                        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                          "%s",
                                          Var->Bool32 ? "true" : "false");
                    }
                    else
                    {
                        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                          "%d", Var->Bool32);
                    }
                } break;

-   full code below: 

                internal memory_index
                DEBUGVariableToText(char *Buffer, char *End, debug_variable *Var, u32 Flags)
                {
                    char *At = Buffer;

                    if(Flags & DEBUGVarToText_AddDebugUI)
                    {
                        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                          "#define DEBUGUI_");
                    }

                    if(Flags & DEBUGVarToText_AddName)
                    {
                        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                          "%s%s ", Var->Name, (Flags & DEBUGVarToText_Colon) ? ":" : "");
                    }
                    
                    switch(Var->Type)
                    {
                        case DebugVariableType_Bool32:                
                        {
                            if(Flags & DEBUGVarToText_PrettyBools)
                            {
                                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                                  "%s",
                                                  Var->Bool32 ? "true" : "false");
                            }
                            else
                            {
                                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                                  "%d", Var->Bool32);
                            }
                        } break;

                        case DebugVariableType_Int32:
                            ...
                        case DebugVariableType_UInt32:                
                            ...
                        case DebugVariableType_V2:
                            ...
                        case DebugVariableType_V3:
                            ...
                        case DebugVariableType_V4:
                            ...

                        case DebugVariableType_Real32:                
                        {
                            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                              "%f", Var->Real32);
                            if(Flags & DEBUGVarToText_FloatSuffix)
                            {
                                *At++ = 'f';
                            }
                        } break;

                        case DebugVariableType_Group:
                        {
                        } break;

                        InvalidDefaultCase;
                    }

                    if(Flags & DEBUGVarToText_LineFeedEnd)
                    {
                        *At++ = '\n';
                    }

                    if(Flags & DEBUGVarToText_NullTerminator)
                    {
                        *At++ = 0;
                    }
                    
                    return(At - Buffer);
                }



as a matter of fact, if you look at the DrawDebugMainMenu(); function, we also output DEBUGVariables as strings.
and there we dont pass the FloatSuffix flag. 

                handmade_debug_variables.h
                
                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    ...
                    ...

                    while(Var)
                    {
                        ...
                        DEBUGVariableToText(Text, Text + sizeof(Text), Var,
                                            DEBUGVarToText_AddName|
                                            DEBUGVarToText_NullTerminator|
                                            DEBUGVarToText_Colon|
                                            DEBUGVarToText_PrettyBools);

                        .....................................................
                        ............ Get Next debug Variable ................
                        .....................................................
                    }
                }


46:30
Casey defined a list of flags in handmade_debug.h

                handmade_debug.h 

                enum debug_variable_to_text_flag
                {
                    DEBUGVarToText_AddDebugUI = 0x1,
                    DEBUGVarToText_AddName = 0x2,
                    DEBUGVarToText_FloatSuffix = 0x4,
                    DEBUGVarToText_LineFeedEnd = 0x8,
                    DEBUGVarToText_NullTerminator = 0x10,
                    DEBUGVarToText_Colon = 0x20,
                    DEBUGVarToText_PrettyBools = 0x40,
                };

essentially, now the coder can have more control on what kind of text they want to output. 


Q/A
1:17:20
how would you implement your own string without it being null terminated?
bundle the char* with a counter variable?

Yes.



