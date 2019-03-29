Handmade Hero Day 250 - Cleaning Up Debug GUIDs

Summary:
cleaned up debug_event and how we generate GUID for debug_event in the UniqueFileCounterString__ macro function
explained why Casey added dtForFrame into the game_input struct 

Keyword:
Debug System

14:49
just to recap, currently for every RecordDebugEvent, we are using __FILE__, __LINE__, __COUNTER__, Name 
to generate a GUID

                #define UniqueFileCounterString__(A, B, C, D) A "(" #B ")." #C ": " D
                #define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
                #define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

and we are storing it in the debug_event


                #define RecordDebugEvent(EventType, GUIDInit)           \
                u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
                        u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;            \
                        Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0]));   \
                        debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; \
                        Event->Clock = __rdtsc();                       \
                        Event->Type = (u8)EventType;                                    \
                        Event->CoreIndex = 0;                                           \
                        Event->ThreadID = (u16)GetThreadID();                         \
                        Event->GUID = GUIDInit;

the debug_event has a char* GUID as the GUID variable 

                struct debug_event
                {
                    u64 Clock;
                    char *GUID;
                    u16 ThreadID;
                    u16 CoreIndex;
                    u8 Type;
                    union
                    {
                        debug_id DebugID;
                        debug_event *Value_debug_event;

                        b32 Value_b32;
                        s32 Value_s32;
                        u32 Value_u32;
                        r32 Value_r32;
                        v2 Value_v2;
                        v3 Value_v3;
                        v4 Value_v4;
                        rectangle2 Value_rectangle2;
                        rectangle3 Value_rectangle3;
                        bitmap_id Value_bitmap_id;
                        sound_id Value_sound_id;
                        font_id Value_font_id;
                    };
                };

22:12
Casey also gave a recap of our debug System.
we have the debug_events, which are things that go into the buffer that we actually look at. 

then we had 4 things 
       
                struct debug_state
                {
                    ...
                    ...

                    debug_element *ElementHash[1024];
                    debug_view *ViewHash[4096];
                    debug_variable_group *RootGroup;
                    debug_tree TreeSentinel;

                    ...
                    ...
                };

for ui.


30:21
so in our debug system, Casey mentioned that we want to hash the string guid

-   so first as you can see, we first scan through the entire Event->GUID string to produce 
    a hash value. 

    with that hash value, we get the the index into our hash table 

                u32 Index = (HashValue % ArrayCount(DebugState->ElementHash));

-   then we look through our DebugState->ElementHash[index] to see if we have a debug_element that has this GUID. 
    recall we are doing the linked list hash table. so DebugState->ElementHash[index] contains the linked list

-   full code below:
                handmade_debug.cpp

                internal debug_element * GetElementFromEvent(debug_state *DebugState, debug_event *Event)
                {
                    Assert(Event->GUID);
                    
                    u32 HashValue = 0;
                    u32 FileNameCount = 0;
                    u32 NameStartsAt = 0;
                    u32 LineNumber = 0;
                    u32 PipeCount = 0;
                    for(char *Scan = Event->GUID; *Scan; ++Scan)
                    {
                        if(*Scan == '|')
                        {
                            if(PipeCount == 0)
                            {
                                FileNameCount = (u32)(Scan - Event->GUID);
                                LineNumber = atoi(Scan + 1);
                            }
                            else if(PipeCount == 1)
                            {
                                
                            }
                            else
                            {
                                NameStartsAt = (u32)(Scan - Event->GUID + 1);
                            }
                            
                            ++PipeCount;
                        }
                        
                        // TODO(casey): Better hash function
                        HashValue = 65599*HashValue + *Scan;
                    }
                    
                    u32 Index = (HashValue % ArrayCount(DebugState->ElementHash));
                    
                    debug_element *Result = 0;
                    

                    for(debug_element *Chain = DebugState->ElementHash[Index]; Chain; Chain = Chain->NextInHash)
                    {
                        if(StringsAreEqual(Chain->GUID, Event->GUID))
                        {
                            Result = Chain;
                            break;
                        }
                    }

                    .......................................................................
                    ............ produce new debug_element if we dont have it .............
                    .......................................................................
                    return(Result);
                }

41:20
notice that Casey made UniqueFileCounterString__ have "|" so that it is easier to parse 

                #define UniqueFileCounterString__(A, B, C, D) A "|" #B "|" #C "|" D
                #define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
                #define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

this is why in the GetElementFromEvent(); function, in the first for loop, we are scanning for '|' 




51:45
Casey mentioned that. one thing you can do with hot loading on 64 bit system is that since you have "infinite" memory.
you can just load the dll, and never free them. 

for example, if you load a dll for your first run, you get the first string table for your first dll.
then for your second run, you load your 2nd copy of the dll. so we dont free the first dll, that way 
we can still use the string table from the first dll.  


Q/A 

1:11:53
Casey mentioned how intel instruction encoding is kind of like the huffman encoding. the reason why this happened 
is because how the instruction set got extended over time. it is backwards compatiable. 

everytime intel extends functionality, they can do some huffman encoding like schemes to indicate new functionality

1:22:56
someone questioned why 


                typedef struct game_input
                {
                    r32 dtForFrame;

                    game_controller_input Controllers[5];

                    // NOTE(casey): Signals back to the platform layer
                    b32 QuitRequested;

                    // NOTE(casey): For debugging only
                    game_button_state MouseButtons[PlatformMouseButton_Count];
                    r32 MouseX, MouseY, MouseZ;
                    b32 ShiftDown, AltDown, ControlDown;
                } game_input;

why is dtForFrame in the game_input?


if you think about what the clock is, its input to a game. you can think of it as, things coming from the outside:
what the user is typing on the keyboard, what the user is clicking on the mouse and the time it is passing. 
so when we give all the information to the game code, we are saying: here is all the information you need to know 
that happend since last time you did something. 
The time, is just like all "external hardware inputs".


another way to look at it is, what are the things you need to record to playback a user session?
i need keyboard input, user input and the time. So the wall clock is essentially an input device. 
