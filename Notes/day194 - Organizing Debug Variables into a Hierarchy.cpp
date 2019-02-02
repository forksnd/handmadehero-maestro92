Handmade Hero Day 194 - Organizing Debug Variables into a Hierarchy

Summary:
organized the debug variables into a Hierarchy structure
reminds me of a XML or JSON file 

Keyword:
build process



21:51
right now all of our debug_variable in the DebugVariableList will get listed in our Radial debug menu 

Casey wants to categorized into a Hierarchy, where lets say rendering related ones all get grouped together,
or Asset related ones get grouped together.


23:30
as a side note, C and C++ do a very bad job of allowing us enter data. This is the one drawback of C,
where they didnt think through coders would want to enter data with their code. 


26:30
so instead of statically defining the debug data in handmade_debug_variables.h, Casey wanted something more flexible.
So he changed the data definition through a function call in DEBUGStart();

here in calls the DEBUGCreateVariables(); variable

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
        ---------------->   DEBUGCreateVariables(DebugState);
                            ...
                            ...
                        }
                 
                    }
                }


and Casey proceeds to create a DEBUGCreateVariables(); function in handmade_debug_variables.h

                handmade_debug_variables.h

                internal void DEBUGCreateVariables(debug_state *State)
                {
                    DEBUG_VARIABLE_LISTING(DEBUGUI_UseDebugCamera),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_GroundChunkOutlines),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_ParticleTest),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_ParticleGrid),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_UseSpaceOutlines),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_GroundChunkCheckerboards),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_RecomputeGroundChunksOnEXEChange),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_TestWeirdDrawBufferSize),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_FamiliarFollowsHero),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_ShowLightingSamples),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_UseRoomBasedCamera),
                }


27:11
now Casey has the ability to create groups of these Debug Variables 
so you Casey start putting them into 

                DEBUGBeginVariableGroup();
                ...
                ...
                DEBUGEndVariableGroup(&Context);

brackets 

-   full code below:

                handmade_debug_variables.h

                internal void DEBUGCreateVariables(debug_state *State)
                {
                // TODO(casey): Add _distance_ for the debug camera, so we have a float example
                // TODO(casey): Parameterize the fountain?

                    debug_variable_definition_context Context = {};
                    Context.State = State;
                    Context.Arena = &State->DebugArena;
                    Context.Group = DEBUGBeginVariableGroup(&Context, "Root");
                        
                #define DEBUG_VARIABLE_LISTING(Name) DEBUGAddVariable(&Context, #Name, DEBUGUI_##Name)

                    DEBUGBeginVariableGroup(&Context, "Group Chunks");
                    DEBUG_VARIABLE_LISTING(GroundChunkOutlines);
                    DEBUG_VARIABLE_LISTING(GroundChunkCheckerboards);
                    DEBUG_VARIABLE_LISTING(RecomputeGroundChunksOnEXEChange);
                    DEBUGEndVariableGroup(&Context);

                    DEBUGBeginVariableGroup(&Context, "Particles");
                    DEBUG_VARIABLE_LISTING(ParticleTest);
                    DEBUG_VARIABLE_LISTING(ParticleGrid);
                    DEBUGEndVariableGroup(&Context);
                    
                    DEBUGBeginVariableGroup(&Context, "Renderer");
                    {        
                        DEBUG_VARIABLE_LISTING(TestWeirdDrawBufferSize);
                        DEBUG_VARIABLE_LISTING(ShowLightingSamples);

                        DEBUGBeginVariableGroup(&Context, "Camera");
                        {
                            DEBUG_VARIABLE_LISTING(UseDebugCamera);
                            DEBUG_VARIABLE_LISTING(UseRoomBasedCamera);
                        }
                        DEBUGEndVariableGroup(&Context);

                        DEBUGEndVariableGroup(&Context);
                    }

                    DEBUG_VARIABLE_LISTING(FamiliarFollowsHero);
                    DEBUG_VARIABLE_LISTING(UseSpaceOutlines);

                #undef DEBUG_VARIABLE_LISTING

                    State->RootGroup = Context.Group;
                }


33:11
To make the code a bit more clear, Casey created the struct debug_variable_definition_context

                handmade_debug_variables.h
                                
                struct debug_variable_definition_context
                {
                    debug_state *State;
                    memory_arena *Arena;

                    debug_variable *Group;
                };



36:20
Casey wants an Hierarchy, so he starts to define some of these variables
the idea is that we have a Hierarchy of debug_variable.
each debug_variable can be either a boolean or a group 


                enum debug_variable_type
                {
                    DebugVariableType_Boolean,
                    
                    DebugVariableType_Group,
                };

                struct debug_variable_group
                {
                    b32 Expanded;
                    debug_variable *FirstChild;    
                    debug_variable *LastChild;
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
                        debug_variable_group Group;
                    };
                };


this is essentially like defining a json file 
so visually, you have something like:


                "debug_variable_definition_context.Root":
                {
                    "debug_variable1": 
                    {
                        "Type": "DebugVariableType_Boolean",
                        "Bool32": true,
                    },


                        |   ^
                 Next   |   |  Parent
                        v   |


                    "debug_variable2": 
                    {
                        "Type": "DebugVariableType_Group",
                        "Group":
                            {
                                "debug_variable3": 
                                {
                                    "Type": "DebugVariableType_Boolean",
                                    "Bool32": true,
                                },


                                        |   ^
                                 Next   |   |  Parent
                                        v   |


                                "debug_variable4": 
                                {
                                    "Type": "DebugVariableType_Boolean",
                                    "Bool32": true,
                                }
                            }
                    }


                        |   ^
                 Next   |   |  Parent
                        v   |


                    "debug_variable5": 
                    {
                        "Type": "DebugVariableType_Boolean",
                        "Bool32": true,
                    },
                }


41:06
Casey proceeds to write these functions.
should be straightforwards, its pretty much adding nodes to a linked-list

                internal debug_variable *
                DEBUGAddVariable(debug_variable_definition_context *Context, debug_variable_type Type, char *Name)
                {    
                    debug_variable *Var = PushStruct(Context->Arena, debug_variable);
                    Var->Type = Type;
                    Var->Name = Name;
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

                internal debug_variable *
                DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char *Name)
                {
                    debug_variable *Group = DEBUGAddVariable(Context, DebugVariableType_Group, Name);
                    Group->Group.Expanded = false;
                    Group->Group.FirstChild = Group->Group.LastChild = 0;

                    Context->Group = Group;

                    return(Group);
                }

                internal debug_variable *
                DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, b32 Value)
                {
                    debug_variable *Var = DEBUGAddVariable(Context, DebugVariableType_Boolean, Name);
                    Var->Bool32 = Value;

                    return(Var);
                }

                internal void
                DEBUGEndVariableGroup(debug_variable_definition_context *Context)
                {
                    Assert(Context->Group);

                    Context->Group = Context->Group->Parent;
                }


50:44
so now debug_state has access to our debug variables 
                
                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    debug_variable *RootGroup;
                    
                    ... 
                };




then in our WriteHandmadeConfig();, we go through our Root and output everything into our config file 
its literally iterating through every node and writing it into the config file.
should be pretty straightforward.

-   notice that for every level that we go down, we would add '    '. in which we do 

                *At++ = ' ';
                *At++ = ' ';
                *At++ = ' ';
                *At++ = ' ';  

-   notice that for every group node that we encouter, we would print out a line of comments 

    that is why you see 
                // Group Chunks

                // Particles

                ...
                ...

-   full code below:
                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState)
                {
                    // TODO(casey): Need a giant buffer here!
                    char Temp[4096];
                    char *At = Temp;
                    char *End = Temp + sizeof(Temp);

                    int Depth = 0;
                    debug_variable *Var = DebugState->RootGroup->Group.FirstChild;
                    while(Var)
                    {
                        // TODO(casey): Other variable types!
                        for(int Indent = 0;
                            Indent < Depth;
                            ++Indent)
                        {
                            *At++ = ' ';
                            *At++ = ' ';
                            *At++ = ' ';
                            *At++ = ' ';                
                        }
                        switch(Var->Type)
                        {
                            case DebugVariableType_Boolean:                
                            {
                                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                                  "#define DEBUGUI_%s %d\n", Var->Name, Var->Bool32);
                            } break;

                            case DebugVariableType_Group:
                            {
                                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                                  "// %s\n", Var->Name);
                            } break;
                        }

                        if(Var->Type == DebugVariableType_Group)
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
                    Platform.DEBUGWriteEntireFile("../code/handmade_config.h", (u32)(At - Temp), Temp);

                    if(!DebugState->Compiling)
                    {
                        DebugState->Compiling = true;
                        DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("..\\code", "c:\\windows\\system32\\cmd.exe", "/C build.bat");
                    }
                }


so with this, our config file now looks like: 

                handmade_config.h

                // Group Chunks
                    #define DEBUGUI_GroundChunkOutlines 0
                    #define DEBUGUI_GroundChunkCheckerboards 0
                    #define DEBUGUI_RecomputeGroundChunksOnEXEChange 1
                // Particles
                    #define DEBUGUI_ParticleTest 0
                    #define DEBUGUI_ParticleGrid 0
                // Renderer
                    #define DEBUGUI_TestWeirdDrawBufferSize 0
                    #define DEBUGUI_ShowLightingSamples 0
                    // Camera
                        #define DEBUGUI_UseDebugCamera 0
                        #define DEBUGUI_UseRoomBasedCamera 0
                #define DEBUGUI_FamiliarFollowsHero 0
                #define DEBUGUI_UseSpaceOutlines 0

