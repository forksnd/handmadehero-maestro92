Handmade Hero Day 202 - Multiply Appearing Debug Values

Summary:
briefly mentioned how viual studio browse info work 

finished up the debug_tree construction in our debug system.
used a stack to help with debug_stree construction. The stack is mainly used for remembering parent pointers


Keyword:
debug system, visual studio 


1:04
Casey is talking about how visual studio browse info

the browse info usually wont be turned on if you dont load a project file.


so on the MSDN spec it says
https://docs.microsoft.com/en-us/cpp/build/reference/fr-fr-create-dot-sbr-file?view=vs-2017

During the build process, the Microsoft Browse Information File Maintenance Utility (BSCMAKE); uses these files to create a .BSC file, 
which is used to display browse information.


but Casey found out that even with that, you still need a project to have "F12 look up definition" working





14:58
Casey changed the debug_variable_array

                handmade_debug.h

                struct debug_variable_array
                {
                    u32 Count;
                    debug_variable *Vars;
                };

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
                        debug_profile_settings Profile;
                        debug_bitmap_display BitmapDisplay;
                        debug_variable_array VarArray;
                    };
                };


to a doubly linked list structure 

                struct debug_variable_link
                {
                    debug_variable_link *Next;
                    debug_variable_link *Prev;
                    debug_variable *Var;
                };

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
                        debug_profile_settings Profile;
                        debug_bitmap_display BitmapDisplay;
                        debug_variable_link VarGroup;
                    };
                };




25:28
so previously, when we call BeginVariableGroup(); and EndVariableGroup();

                struct debug_variable_definition_context
                {
                    debug_state *State;
                    memory_arena *Arena;

                    debug_variable *Group;
                };

                internal void DEBUGCreateVariables(debug_variable_definition_context *Context)
                {

                    ...
                    ...

                    DEBUGBeginVariableGroup(Context, "Group Chunks");
                    DEBUG_VARIABLE_LISTING(GroundChunkOutlines);
                    DEBUG_VARIABLE_LISTING(GroundChunkCheckerboards);
                    DEBUG_VARIABLE_LISTING(RecomputeGroundChunksOnEXEChange);
                    DEBUGEndVariableGroup(Context);

                    DEBUGBeginVariableGroup(Context, "Particles");
                    DEBUG_VARIABLE_LISTING(ParticleTest);
                    DEBUG_VARIABLE_LISTING(ParticleGrid);
                    DEBUGEndVariableGroup(Context);

                    ...
                }

we are always adding the BeginVariableGroup(); at debug_variable_definition_context.Group.
so when we call BeginVariableGroup(); debug_variable_definition_context.Group will point to there.



when we call BeginVariableGroup(); we go down when level, 

when we call DEBUGEndVariableGroup(); debug_variable_definition_context.Group will go back up a level 

                internal void DEBUGEndVariableGroup(debug_variable_definition_context *Context)
                {
                    Assert(Context->Group);

                    Context->Group = Context->Group->Parent;
                }

however, this implementation requires us to always store a parent pointer. so that DEBUGEndVariableGroup(); can always go back up.


instead of storing a parent pointer, Casey resort to using a stack. so whenever we go down one level,
we just put it on the stack. Then we we call DEBUGEndVariableGroup(); and we have to go back to our parent, we just 
find our parent from the stack.

                #define DEBUG_MAX_VARIABLE_STACK_DEPTH 64
                struct debug_variable_definition_context
                {
                    debug_state *State;
                    memory_arena *Arena;

                    u32 GroupDepth;
                    debug_variable* GroupStack[DEBUG_MAX_VARIABLE_STACK_DEPTH];
                };





27:21
so in DEBUGBeginVariableGroup(); we just go down one level "Context->GroupStack[++Context->GroupDepth] = Group;"

                internal debug_variable *
                DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char *Name)
                {
                    debug_variable *Group = DEBUGAddVariable(Context->State, DebugVariableType_VarGroup, Name);
                    DLIST_INIT(&Group->VarGroup);

                    ...
                    Context->GroupStack[++Context->GroupDepth] = Group;
                    
                    return(Group);
                }

and in DEBUGEndVariableGroup(); we go back up one level

                internal void
                DEBUGEndVariableGroup(debug_variable_definition_context *Context)
                {
                    Assert(Context->GroupDepth > 0);
                    --Context->GroupDepth;
                }



the rest of the stream is essentially code clean up

